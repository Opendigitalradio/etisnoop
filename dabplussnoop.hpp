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

/* From the spec:

subchannel_index = MSC sub-channel size (kbps) / 8
audio_super_frame_size (bytes) = subchannel_index * 110

// Derived from
// au_start[n] = au_start[n - 1] + au_size[n - 1] + 2;
// 2 bytes for CRC
au_size[n] = au_start[n+1] - au_start[n] - 2;

he_aac_super_frame(subchannel_index)
{
    he_aac_super_frame_header()
    {
        header_firecode       16
        rfa                    1
        dac_rate               1
        sbr_flag               1
        aac_channel_mode       1
        ps_flag                1
        mpeg_surround_config   3

        // end of audio parameters
        if ((dac_rate == 0) && (sbr_flag == 1)) num_aus = 2;
        // AAC core sampling rate 16 kHz
        if ((dac_rate == 1) && (sbr_flag == 1)) num_aus = 3;
        //  AAC core sampling rate 24 kHz
        if ((dac_rate == 0) && (sbr_flag == 0)) num_aus = 4;
        // AAC core sampling rate 32 kHz
        if ((dac_rate == 1) && (sbr_flag == 0)) num_aus = 6;
        // AAC core sampling rate 48 kHz

        for (n = 1; n < num_aus; n++) {
            au_start[n];      12
        }

        if !((dac_rate == 1) && (sbr_flag == 1))
        alignment              4
    }

    for (n = 0; n < num_aus; n++) {
        au[n]                  8 * au_size[n]
        au_crc[n]             16
    }

}
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <vector>
#include "faad_decoder.hpp"

#ifndef __DABPLUSSNOOP_H_
#define __DABPLUSSNOOP_H_


class DabPlusSnoop
{
    public:
        DabPlusSnoop() :
            m_index(0),
            m_subchannel_index(0),
            m_data(0),
            m_raw_data_stream_fd(NULL) {}

        void set_subchannel_index(unsigned subchannel_index)
        {
            m_subchannel_index = subchannel_index;
        }

        void set_index(int index)
        {
            m_index = index;
        }

        void push(uint8_t* streamdata, size_t streamsize);

        void close(void);

    private:
        /* Data needed for FAAD */
        FaadDecoder m_faad_decoder;
        int  m_index;

        bool m_ps_flag;
        bool m_aac_channel_mode;
        bool m_dac_rate;
        bool m_sbr_flag;
        int  m_mpeg_surround_config;

        /* Functions */

        bool seek_valid_firecode(void);
        bool decode(void);
        bool extract_au(std::vector<int> au_start);
        bool analyse_au(std::vector<std::vector<uint8_t> >& aus);

        unsigned m_subchannel_index;
        std::vector<uint8_t> m_data;

        FILE* m_raw_data_stream_fd;
};

#endif

