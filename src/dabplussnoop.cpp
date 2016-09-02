/*
    Copyright (C) 2014, 2015 Matthias P. Braendli (http://www.opendigitalradio.org)

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

    dabplussnoop.cpp
          Parse DAB+ frames from a ETI file

    Authors:
         Matthias P. Braendli <matthias@mpb.li>
         Mathias Coinchon <coinchon@yahoo.com>
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <vector>
#include "dabplussnoop.hpp"
extern "C" {
#include "firecode.h"
#include "lib_crc.h"
}
#include "faad_decoder.hpp"
#include "rsdecoder.hpp"

#define DPS_INDENT "\t\t"
#define DPS_PREFIX "DAB+ decode:"

#define DPS_DEBUG 0

using namespace std;

void DabPlusSnoop::push(uint8_t* streamdata, size_t streamsize)
{
    // Try to decode audio
    size_t original_size = m_data.size();
    m_data.resize(original_size + streamsize);
    memcpy(&m_data[original_size], streamdata, streamsize);

    if (seek_valid_firecode()) {
        // m_data now points to a valid header

        if (decode()) {

            // First dump to subchannel file (superframe+parity word)
            if (m_raw_data_stream_fd == NULL) {
                stringstream dump_filename;
                dump_filename << "stream-" << m_index << ".msc";

                m_raw_data_stream_fd = fopen(dump_filename.str().c_str(), "w");

                if (m_raw_data_stream_fd == NULL) {
                    perror("File open failed");
                }
            }

            fwrite(&m_data[0], m_subchannel_index, 120, m_raw_data_stream_fd);

            // We have been able to decode the AUs, now flush vector
            m_data.clear();
        }
    }
}

// Idea and some code taken from Xpadxpert
bool DabPlusSnoop::seek_valid_firecode()
{
    if (m_data.size() < 10) {
        // Not enough data
        return false;
    }

    bool crc_ok = false;
    size_t i;

    for (i = 0; i < m_data.size() - 10; i++) {
        uint8_t* b = &m_data[i];

        // the three bytes after the firecode must not be zero
        // (simple plausibility check to avoid sync in zero byte region)
        if (b[3] != 0x00 || (b[4] & 0xF0) != 0x00) {
            uint16_t header_firecode = (b[0] << 8) | b[1];
            uint16_t calculated_firecode = firecode_crc(b+2, 9);

            if (header_firecode == calculated_firecode) {
                crc_ok = true;
                break;
            }
        }
    }

    if (crc_ok) {
#if DPS_DEBUG
        printf(DPS_PREFIX " Found valid FireCode at %zu\n", i);
#endif
        //erase elements before the header
        m_data.erase(m_data.begin(), m_data.begin() + i);
        return true;
    }
    else {
#if DPS_DEBUG
        printf(DPS_PREFIX " No valid FireCode found\n");
#endif

        m_data.clear();
        return false;
    }
}

bool DabPlusSnoop::decode()
{
#if DPS_DEBUG
    printf(DPS_PREFIX " We have %zu bytes of data\n", m_data.size());
#endif

    const size_t sf_len = m_subchannel_index * 120;
    if (m_subchannel_index && m_data.size() >= sf_len) {
        std::vector<uint8_t> b(sf_len);
        std::copy(m_data.begin(), m_data.begin() + sf_len, b.begin());

        RSDecoder rs_dec;
        int rs_errors = rs_dec.DecodeSuperframe(b, m_subchannel_index);

        if (rs_errors == -1) {
            return false;
        }

        // -- Parse he_aac_super_frame
        // ---- Parse he_aac_super_frame_header
        // ------ Parse firecode and audio params
        uint16_t header_firecode = (b[0] << 8) | b[1];
        uint8_t  audio_params    = b[2];
        int rfa                  = (audio_params & 0x80) ? true : false;
        m_dac_rate               = (audio_params & 0x40) ? true : false;
        m_sbr_flag               = (audio_params & 0x20) ? true : false;
        m_aac_channel_mode       = (audio_params & 0x10) ? true : false;
        m_ps_flag                = (audio_params & 0x08) ? true : false;
        m_mpeg_surround_config   = (audio_params & 0x07);

        int num_aus;
        if (!m_dac_rate && m_sbr_flag) num_aus = 2;
        // AAC core sampling rate 16 kHz
        else if (m_dac_rate && m_sbr_flag) num_aus = 3;
        //  AAC core sampling rate 24 kHz
        else if (!m_dac_rate && !m_sbr_flag) num_aus = 4;
        // AAC core sampling rate 32 kHz
        else if (m_dac_rate && !m_sbr_flag) num_aus = 6;
        // AAC core sampling rate 48 kHz

#if DPS_DEBUG
        printf( DPS_INDENT DPS_PREFIX "\n"
                DPS_INDENT "\tfirecode           0x%x\n"
                DPS_INDENT "\trfa                  %d\n"
                DPS_INDENT "\tdac_rate             %d\n"
                DPS_INDENT "\tsbr_flag             %d\n"
                DPS_INDENT "\taac_channel_mode     %d\n"
                DPS_INDENT "\tps_flag              %d\n"
                DPS_INDENT "\tmpeg_surround_config %d\n"
                DPS_INDENT "\tnum_aus              %d\n",
                header_firecode, rfa, m_dac_rate, m_sbr_flag,
                m_aac_channel_mode, m_ps_flag, m_mpeg_surround_config,
                num_aus);
#else
        // Avoid "unused variable" warning
        (void)header_firecode;
        (void)rfa;
#endif


        // ------ Parse au_start
        auto au_starts = b.begin() + 3;

        vector<uint8_t> au_start_nibbles(0);

        /* Each AU_START is encoded in three nibbles.
         * When we have n AUs, we have n-1 au_start values. */
        for (int i = 0; i < (num_aus-1)*3; i++) {
            if (i % 2 == 0) {
                char nibble = au_starts[i/2] >> 4;

                au_start_nibbles.push_back( nibble );
            }
            else {
                char nibble = au_starts[i/2] & 0x0F;

                au_start_nibbles.push_back( nibble );
            }

        }


        vector<int> au_start(num_aus);

        if (num_aus == 2)
            au_start[0] = 5;
        else if (num_aus == 3)
            au_start[0] = 6;
        else if (num_aus == 4)
            au_start[0] = 8;
        else if (num_aus == 6)
            au_start[0] = 11;


        int nib = 0;
        for (int au = 1; au < num_aus; au++) {
            au_start[au] = au_start_nibbles[nib]   << 8 | \
                           au_start_nibbles[nib+1] << 4 | \
                           au_start_nibbles[nib+2];

            nib += 3;
        }

#if DPS_DEBUG
        printf(DPS_INDENT DPS_PREFIX " AU start\n");
        for (int au = 0; au < num_aus; au++) {
            printf(DPS_INDENT "\tAU[%d] %d 0x%x\n", au,
                    au_start[au],
                    au_start[au]);
        }
#endif

        return extract_au(au_start);
    }
    else {
        return false;
    }
}

bool DabPlusSnoop::extract_au(vector<int> au_start)
{
    vector<vector<uint8_t> > aus(au_start.size());

    // The last entry of au_start must the end of valid
    // AU data. We stop at m_subchannel_index * 110 because
    // what comes after is RS parity
    au_start.push_back(m_subchannel_index * 110);

    bool all_crc_ok = true;

    for (size_t au = 0; au < aus.size(); au++)
    {
#if DPS_DEBUG
        printf(DPS_PREFIX DPS_INDENT
                "Copy au %zu of size %zu\n",
                au,
                au_start[au+1] - au_start[au]-2 );
#endif

        aus[au].resize(au_start[au+1] - au_start[au]-2);
        std::copy(
                m_data.begin() + au_start[au],
                m_data.begin() + au_start[au+1]-2,
                aus[au].begin() );

        /* Check CRC */
        uint16_t au_crc = m_data[au_start[au+1]-2] << 8 | \
                          m_data[au_start[au+1]-1];

        uint16_t calc_crc = 0xFFFF;
        for (vector<uint8_t>::iterator au_data = aus[au].begin();
                au_data != aus[au].end();
                ++au_data) {
            calc_crc = update_crc_ccitt(calc_crc, *au_data);
        }
        calc_crc =~ calc_crc;

        if (calc_crc != au_crc) {
            printf(DPS_INDENT DPS_PREFIX
                    "Erroneous CRC for au %zu: 0x%04x vs 0x%04x\n",
                    au, calc_crc, au_crc);

            all_crc_ok = false;
        }
    }

    if (all_crc_ok) {
        return analyse_au(aus);
    }
    else {
        //discard faulty superframe (to be improved to correct/conceal)
        m_data.clear();
        return false;
    }
}

bool DabPlusSnoop::analyse_au(vector<vector<uint8_t> >& aus)
{
    stringstream ss_filename;

    ss_filename << "stream-" << m_index;

    if (!m_faad_decoder.is_initialised()) {
        m_faad_decoder.open(ss_filename.str(), m_ps_flag,
                m_aac_channel_mode, m_dac_rate, m_sbr_flag,
                m_mpeg_surround_config);
    }

    return m_faad_decoder.decode(aus);
}

void DabPlusSnoop::close()
{
    m_faad_decoder.close();

    if (m_raw_data_stream_fd) {
        fclose(m_raw_data_stream_fd);
    }
}
