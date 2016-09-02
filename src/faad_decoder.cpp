/*
    Copyright (C) 2014 Matthias P. Braendli (http://www.opendigitalradio.org)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    faad_decoder.cpp
        Use libfaad to decode the AAC content of the DAB+ subchannel

    Authors:
         Matthias P. Braendli <matthias@mpb.li>
*/

#include "faad_decoder.hpp"
extern "C" {
#include "wavfile.h"
}
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

FaadDecoder::FaadDecoder() :
    m_data_len(0),
    m_fd(NULL),
    m_initialised(false)
{
}

void FaadDecoder::open(string filename, bool ps_flag, bool aac_channel_mode,
        bool dac_rate, bool sbr_flag, int mpeg_surround_config)
{
    m_filename             = filename;
    m_ps_flag              = ps_flag;
    m_aac_channel_mode     = aac_channel_mode;
    m_dac_rate             = dac_rate;
    m_sbr_flag             = sbr_flag;
    m_mpeg_surround_config = mpeg_surround_config;
}

bool FaadDecoder::decode(vector<vector<uint8_t> > aus)
{
    for (size_t au_ix = 0; au_ix < aus.size(); au_ix++) {

        vector<uint8_t>& au = aus[au_ix];

        NeAACDecFrameInfo hInfo;
        int16_t* outBuffer;

        if (!m_initialised) {
            /* AudioSpecificConfig structure (the only way to select 960 transform here!)
             *
             *  00010 = AudioObjectType 2 (AAC LC)
             *  xxxx  = (core) sample rate index
             *  xxxx  = (core) channel config
             *  100   = GASpecificConfig with 960 transform
             *
             * SBR: implicit signaling sufficient - libfaad2 automatically assumes SBR on sample rates <= 24 kHz
             * => explicit signaling works, too, but is not necessary here
             *
             * PS:  implicit signaling sufficient - libfaad2 therefore always uses stereo output (if PS support was enabled)
             * => explicit signaling not possible, as libfaad2 does not support AudioObjectType 29 (PS)
             */

            int core_sr_index = m_dac_rate ? (m_sbr_flag ? 6 : 3) : (m_sbr_flag ? 8 : 5);   // 24/48/16/32 kHz
            int core_ch_config = get_aac_channel_configuration();
            if(core_ch_config == -1) {
                printf("Unrecognized mpeg surround config (ignored): %d\n", m_mpeg_surround_config);
                return false;
            }

            uint8_t asc[2];
            asc[0] = 0b00010 << 3 | core_sr_index >> 1;
            asc[1] = (core_sr_index & 0x01) << 7 | core_ch_config << 3 | 0b100;


            long unsigned samplerate;
            unsigned char channels;

            long int init_result = NeAACDecInit2(m_faad_handle.decoder, asc, sizeof(asc), &samplerate, &channels);
            if(init_result != 0) {
                /* If some error initializing occured, skip the file */
                printf("Error initializing decoder library: %s\n", NeAACDecGetErrorMessage(-init_result));
                NeAACDecClose(m_faad_handle.decoder);
                return false;
            }

            m_initialised = true;
        }

        outBuffer = (int16_t *)NeAACDecDecode(m_faad_handle.decoder, &hInfo, &au[0], au.size());
        assert(outBuffer != NULL);

        m_sample_rate = hInfo.samplerate;
        m_channels    = hInfo.channels;
        size_t samples  = hInfo.samples;

#if 0
        printf("bytes consumed %d\n", (int)(hInfo.bytesconsumed));
        printf("samplerate = %d, samples = %zu, channels = %d,"
                " error = %d, sbr = %d\n", m_sample_rate, samples,
                m_channels, hInfo.error, hInfo.sbr);
        printf("header = %d\n", hInfo.header_type);
#endif

        if (hInfo.error != 0) {
            printf("FAAD Warning: %s\n",
                    faacDecGetErrorMessage(hInfo.error));
            return false;
        }

        if (m_fd == NULL) {
            stringstream ss;
            ss << m_filename << ".wav";
            m_fd = wavfile_open(ss.str().c_str(), m_sample_rate);
        }

        if (samples) {
            if (m_channels == 1) {
                int16_t *buffer = (int16_t *)alloca (2 * samples);
                size_t i;
                for (i = 0; i < samples; i ++) {
                    buffer [2 * i]  = ((int16_t *)outBuffer) [i];
                    buffer [2 * i + 1] = buffer [2 * i];
                }
                wavfile_write(m_fd, buffer, 2*samples);
            }
            else if (m_channels == 2) {
                wavfile_write(m_fd, outBuffer, samples);
            }
            else {
                printf("Cannot handle %d channels\n", m_channels);
            }
        }

    }
    return true;
}

void FaadDecoder::close()
{
    if (m_initialised) {
        wavfile_close(m_fd);
    }
}

int FaadDecoder::get_aac_channel_configuration()
{
    switch(m_mpeg_surround_config) {
    case 0:     // no surround
        return m_aac_channel_mode ? 2 : 1;
    case 1:     // 5.1
        return 6;
    case 2:     // 7.1
        return 7;
    default:
        return -1;
    }
}


