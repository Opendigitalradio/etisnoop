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

#include "faad_decoder.h"
#include "utils.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

FaadDecoder::FaadDecoder() :
            m_filename(""),
            m_ps_flag(0),
            m_aac_channel_mode(0),
            m_dac_rate(0),
            m_sbr_flag(0),
            m_mpeg_surround_config(0),
            m_initialised(false)
{
    m_decoder = NULL;
}

FaadDecoder::FaadDecoder(string filename, bool ps_flag, bool aac_channel_mode,
        bool dac_rate, bool sbr_flag, int mpeg_surround_config) :
            m_filename(filename),
            m_ps_flag(ps_flag),
            m_aac_channel_mode(aac_channel_mode),
            m_dac_rate(dac_rate),
            m_sbr_flag(sbr_flag),
            m_mpeg_surround_config(mpeg_surround_config),
            m_initialised(false)
{

    m_decoder = NeAACDecOpen();

    m_fd = fopen(m_filename.c_str(), "w");

#if 0
    NeAACDecConfigurationPtr config;

    config = NeAACDecGetCurrentConfiguration(m_decoder);
    if (def_srate)
        config->defSampleRate = def_srate;
    config->defObjectType = object_type;
    config->outputFormat = outputFormat;
    config->downMatrix = downMatrix;
    config->useOldADTSFormat = old_format;
    //config->dontUpSampleImplicitSBR = 1;
    NeAACDecSetConfiguration(m_decoder, config);
#endif
}

size_t FaadDecoder::Decode(vector<vector<uint8_t> > aus)
{
    if (!m_decoder) {
        return 2;
    }

    /* ADTS header creation taken from SDR-J */
    adts_fixed_header fh;
    adts_variable_header vh;

    fh.syncword             = 0xfff;
    fh.id                   = 0;
    fh.layer                = 0;
    fh.protection_absent    = 1;
    fh.profile_objecttype   = 0;    // aac main - 1
    fh.private_bit          = 0;    // ignored when decoding
    fh.original_copy        = 0;
    fh.home                 = 0;
    vh.copyright_id_bit     = 0;
    vh.copyright_id_start   = 0;
    vh.adts_buffer_fullness = 1999; // ? according to OpenDab
    vh.no_raw_data_blocks   = 0;

    uint8_t d_header[10];
    d_header[0]     = fh.syncword >> 4;
    d_header[1]     = (fh.syncword & 0xf) << 4;
    d_header[1]    |= fh.id << 3;
    d_header[1]    |= fh.layer << 1;
    d_header[1]    |= fh.protection_absent;
    d_header[2]     = fh.profile_objecttype << 6;
    //  sampling frequency index filled in dynamically
    d_header[2]    |= fh.private_bit << 1;
    //  channel configuration filled in dynamically
    d_header[3]     = fh.original_copy << 5;
    d_header[3]    |= fh.home << 4;
    d_header[3]    |= vh.copyright_id_bit << 3;
    d_header[3]    |= vh.copyright_id_start << 2;
    //  framelength filled in dynamically
    d_header[4]     = 0;
    d_header[5]     = vh.adts_buffer_fullness >> 6;
    d_header[6]     = (vh.adts_buffer_fullness & 0x3f) << 2;
    d_header[6]    |= vh.no_raw_data_blocks;

    if (!m_dac_rate && m_sbr_flag) fh.sampling_freq_idx = 8;
    // AAC core sampling rate 16 kHz
    else if (m_dac_rate && m_sbr_flag) fh.sampling_freq_idx = 6;
    //  AAC core sampling rate 24 kHz
    else if (m_dac_rate && !m_sbr_flag) fh.sampling_freq_idx = 5;
    // AAC core sampling rate 32 kHz
    else if (m_dac_rate && !m_sbr_flag) fh.sampling_freq_idx = 3;
    // AAC core sampling rate 48 kHz

    setBits (&d_header[2], fh.sampling_freq_idx, 2, 4);

    if (m_mpeg_surround_config == 0) {
        if (m_sbr_flag && !m_aac_channel_mode && m_ps_flag)
            fh.channel_conf   = 2;
        else
            fh.channel_conf   = 1 << (m_aac_channel_mode ? 1 : 0);
    }
    else if (m_mpeg_surround_config == 1) {
            fh.channel_conf       = 6;
    }
    else {
        printf("Unrecognized mpeg surround config (ignored)\n");
        return 1;
    }

    setBits (&d_header[2], fh.channel_conf, 7, 3);


    for (size_t au_ix = 0; au_ix < aus.size(); au_ix++) {

        vector<uint8_t>& au = aus[au_ix];

        uint8_t helpBuffer[960];
        memset(helpBuffer, 0, sizeof(helpBuffer));

        // Set length in header
        vh.aac_frame_length = au.size();
        setBits(&d_header[3], vh.aac_frame_length, 6, 13);

        memcpy(helpBuffer, d_header, 7 * sizeof(uint8_t));
        memcpy(&helpBuffer[7],
                &au[0], vh.aac_frame_length * sizeof (uint8_t));



        long unsigned samplerate;
        unsigned char channels;
        NeAACDecFrameInfo hInfo;
        int16_t* outBuffer;

        if (!m_initialised) {
            int len;

            if ((len = NeAACDecInit(m_decoder, helpBuffer,
                            vh.aac_frame_length, &samplerate, &channels)) < 0)
            {
                /* If some error initializing occured, skip the file */
                printf("Error initializing decoder library (%d).\n",
                        len);
                NeAACDecClose(m_decoder);
                return 1;
            }

            m_initialised = true;

            outBuffer = (int16_t *)NeAACDecDecode(
                    m_decoder, &hInfo,
                    helpBuffer + len, vh.aac_frame_length - len );
        }
        else {
            outBuffer = (int16_t *)NeAACDecDecode(
                    m_decoder, &hInfo,
                    helpBuffer, vh.aac_frame_length );
        }

        int sample_rate = hInfo.samplerate;
        size_t samples  = hInfo.samples;

        printf("bytes consumed %d\n", (int)(hInfo.bytesconsumed));
        printf("samplerate = %d, samples = %zu, channels = %d,"
                " error = %d, sbr = %d\n", sample_rate, samples,
                hInfo.channels, hInfo.error, hInfo.sbr);
        printf("header = %d\n", hInfo.header_type);

        //channels = hInfo.channels;
        if (hInfo.error != 0) {
            printf("FAAD Warning: %s\n",
                    faacDecGetErrorMessage(hInfo.error));
            return 1;
        }

        if (channels == 2) {
            fwrite(outBuffer, 1, samples, m_fd);
        }
        else {
            if (channels == 1) {
                int16_t *buffer = (int16_t *)alloca (2 * samples);
                int16_t i;
                for (i = 0; i < samples; i ++) {
                    buffer [2 * i]  = ((int16_t *)outBuffer) [i];
                    buffer [2 * i + 1] = buffer [2 * i];
                }
                fwrite(outBuffer, 2, samples, m_fd);
            }
            else {
                printf("Cannot handle these channels\n");
            }
        }

    }
    return 0;
}


FaadDecoder::~FaadDecoder()
{
    if (m_decoder) {
        NeAACDecClose(m_decoder);

        fclose(m_fd);
    }
}

