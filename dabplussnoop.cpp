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
#include <cassert>
#include <sstream>
#include <algorithm>
#include <vector>
#include "dabplussnoop.h"
#include "firecode.h"
#include "lib_crc.h"

#define DPS_INDENT "\t\t"
#define DPS_PREFIX "DAB+ decode:"

#define DPS_DEBUG 0

using namespace std;

void DabPlusSnoop::push(uint8_t* streamdata, size_t streamsize)
{
    size_t original_size = m_data.size();
    m_data.resize(original_size + streamsize);

    memcpy(&m_data[original_size], streamdata, streamsize);

    if (seek_valid_firecode()) {
        // m_data now points to a valid header
        if (decode()) {
            // We have been able to decode the AUs
            m_data.clear();
        }
    }
}

// Idea and some code taken from Xpadxpert
bool DabPlusSnoop::seek_valid_firecode()
{
    if (m_data.size() < 10) {
        // Not enough data
        return -1;
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

    if (m_subchannel_index && m_data.size() >= m_subchannel_index * 120) {

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

        printf( DPS_INDENT DPS_PREFIX "\n"
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

        vector<uint8_t> au_start_nibbles(0);

        /* Each AU_START is encoded in three nibbles.
         * When we have n AUs, we have n-1 au_start values. */
        for (int i = 0; i < (num_aus-1)*3; i++) {
            if (i % 2 == 0) {
                char nibble = b[i/2] >> 4;

                au_start_nibbles.push_back( nibble );
            }
            else {
                char nibble = b[i/2] & 0x0F;

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
                    "Erroneous CRC for au %zu\n", au);

            all_crc_ok = false;
        }
    }

    if (all_crc_ok) {
        return analyse_au(aus);
    }
    else {
        return false;
    }
}

bool DabPlusSnoop::analyse_au(vector<vector<uint8_t> >& aus)
{
    for (size_t i_au = 0; i_au < aus.size(); i_au++) {
        size_t i = 0;

        vector<uint8_t>& au = aus[i_au];

        bool continue_au = true;
        while (continue_au && i < au.size()) {
            printf("R at %zu\n", i);
            int id_syn_ele = (au[i] & 0xE0) >> 5;

            /* Debugging print */
            stringstream ss;
            ss << DPS_INDENT << "\tID_SYN_ELE: ";

            switch (id_syn_ele) {
                case ID_SCE:  ss << "Single Channel Element"; break;
                case ID_CPE:  ss << "Channel Pair Element"; break;
                case ID_CCE:  ss << "Coupling Channel Element"; break;
                case ID_LFE:  ss << "LFE Channel Element"; break;
                case ID_DSE:  ss << "Data Stream Element"; break;
                case ID_PCE:  ss << "Program Config Element"; break;
                case ID_FIL:  ss << "Fill Element"; break;
                case ID_END:  ss << "Terminator"; break;
                case ID_EXT:  ss << "Extension Payload"; break;
                case ID_SCAL: ss << "AAC scalable element"; break;
                default:      ss << "Unknown (" << id_syn_ele << ")"; break;
            }

            int element_instance_tag = (au[i] & 0x78) >> 1;
            ss << " [" << element_instance_tag << "]";


            // Keep track of index increment in bits
            size_t inc = 7; // eat id_syn_ele, element_instance_tag

            if (id_syn_ele == ID_DSE) {
                bool data_byte_align_flag = (au[i] & 0x01);
                inc++;

                ss << "\n" DPS_INDENT "\t\t";
                if (data_byte_align_flag) {
                    ss << " <byte align flag>";
                }


                uint8_t count = au[1];
                int cnt = count;
                inc += 8;

                if (count == 255) {
                    uint8_t esc_count = au[2];
                    inc += 8;

                    cnt += esc_count;
                }

                ss << " cnt:" << cnt;
                inc += 8*cnt;

            }
            else
            {
                continue_au = false;
            }

            printf("%s\n", ss.str().c_str());

            assert (inc % 8 == 0);
            i += inc / 8;
        }
    }

    return true;
}

