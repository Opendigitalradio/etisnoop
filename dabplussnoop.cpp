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

    dabplussnoop.cpp
          Parse DAB+ frames from a ETI file

    Authors:
         Matthias P. Braendli <matthias@mpb.li>
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include "dabplussnoop.h"
#include "firecode.h"

#define DPS_INDENT "\t\t"
#define DPS_PREFIX "DABPLUS:"

using namespace std;

void DabPlusSnoop::push(uint8_t* streamdata, size_t streamsize)
{
    size_t original_size = m_data.size();
    m_data.resize(original_size + streamsize);

    memcpy(&m_data[original_size], streamdata, streamsize);

    check();
}

// Idea and some code taken from Xpadxpert
int DabPlusSnoop::check()
{
    if (m_data.size() < 10) {
        // Not enough data
        return -1;
    }

    bool crc_ok = false;
    int i;

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
        fprintf(stderr, DPS_PREFIX " Found valid FireCode at %d\n", i);

        m_data.erase(m_data.begin(), m_data.begin() + i);
        return decode();
    }
    else {
        fprintf(stderr, DPS_PREFIX " No valid FireCode found\n");
        return -1;
    }
}

int DabPlusSnoop::decode()
{
    if (m_subchannel_index && m_data.size() >= m_subchannel_index * 110) {
        fprintf(stderr, DPS_PREFIX " We have %zu bytes of data\n", m_data.size());

        uint8_t* b = &m_data[0];

        // -- Parse he_aac_super_frame
        // ---- Parse he_aac_super_frame_header
        // ------ Parse firecode and audio params
        uint16_t header_firecode = (b[0] << 8) | b[1];
        uint8_t  audio_params    = b[2];
        int rfa                  = (audio_params & 0x80) ? 1 : 0;
        int dac_rate             = (audio_params & 0x40) ? 1 : 0;
        int sbr_flag             = (audio_params & 0x20) ? 1 : 0;
        int aac_channel_mode     = (audio_params & 0x10) ? 1 : 0;
        int ps_flag              = (audio_params & 0x08) ? 1 : 0;
        int mpeg_surround_config = (audio_params & 0x07) ? 1 : 0;

        int num_aus;
        if ((dac_rate == 0) && (sbr_flag == 1)) num_aus = 2;
        // AAC core sampling rate 16 kHz
        else if ((dac_rate == 1) && (sbr_flag == 1)) num_aus = 3;
        //  AAC core sampling rate 24 kHz
        else if ((dac_rate == 0) && (sbr_flag == 0)) num_aus = 4;
        // AAC core sampling rate 32 kHz
        else if ((dac_rate == 1) && (sbr_flag == 0)) num_aus = 6;
        // AAC core sampling rate 48 kHz

        fprintf(stderr,
                DPS_INDENT DPS_PREFIX "\n"
                DPS_INDENT "\tfirecode           0x%x\n"
                DPS_INDENT "\trfa                  %d\n"
                DPS_INDENT "\tdac_rate             %d\n"
                DPS_INDENT "\tsbr_flag             %d\n"
                DPS_INDENT "\taac_channel_mode     %d\n"
                DPS_INDENT "\tps_flag              %d\n"
                DPS_INDENT "\tmpeg_surround_config %d\n"
                DPS_INDENT "\tnum_aus              %d\n",
                header_firecode, rfa, dac_rate, sbr_flag, aac_channel_mode,
                ps_flag, mpeg_surround_config, num_aus);


        // ------ Parse au_start
        b += 3;

        vector<uint8_t> au_start_nibbles(3*(num_aus-1));

        size_t nib = 0;
        for (int i = 0; i < (num_aus-1)*3/2; i++) {
            au_start_nibbles[nib]   = b[i] >> 4;
            au_start_nibbles[nib+1] = 0x0F & (b[i]);
            nib += 2;
            fprintf(stderr, "0x%1x 0x%1x ", b[i] >> 4, 0x0F & b[i]);
        }
        fprintf(stderr, "\n");

        vector<int> au_start(num_aus);

        if (num_aus == 2)
            au_start[0] = 5;
        else if (num_aus == 3)
            au_start[0] = 6;
        else if (num_aus == 4)
            au_start[0] = 8;
        else if (num_aus == 6)
            au_start[0] = 11;


        nib = 0;
        fprintf(stderr, DPS_INDENT DPS_PREFIX " AU start\n");
        for (int au = 1; au < num_aus; au++) {
            au_start[au] = au_start_nibbles[nib]   << 8 | \
                           au_start_nibbles[nib+1] << 4 | \
                           au_start_nibbles[nib+2];

            nib += 3;
        }

        for (int au = 0; au < num_aus; au++) {
            fprintf(stderr, DPS_INDENT "\tAU[%d] %d 0x%x\n", au,
                    au_start[au],
                    au_start[au]);
        }

        m_data.clear();

        return 0;
    }
    else {
        return -1;
    }
}

