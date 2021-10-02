/*
    Copyright (C) 2017 Matthias P. Braendli (http://www.opendigitalradio.org)

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
          Extract MSC data and parse DAB+ frames from a ETI file

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

#pragma once

// DabPlusSnoop is responsible for decoding DAB+ audio
class DabPlusSnoop {
    public:
        void set_subchannel_index(unsigned subchannel_index) {
            m_subchannel_index = subchannel_index;
        }

        void enable_wav_file_output(bool enable) {
            m_write_to_wav_file = enable;
        }

        void push(uint8_t* streamdata, size_t streamsize);

        audio_statistics_t get_audio_statistics(void) const;

        int subchid = -1;

    private:
        /* Data needed for FAAD */
        FaadDecoder m_faad_decoder;
        bool m_write_to_wav_file = false;

        bool m_ps_flag = false;
        bool m_aac_channel_mode = false;
        bool m_dac_rate = false;
        bool m_sbr_flag = false;
        int  m_mpeg_surround_config = false;

        /* Functions */

        bool seek_valid_firecode(void);
        bool decode(void);
        bool extract_au(std::vector<int> au_start);
        bool analyse_au(std::vector<std::vector<uint8_t> >& aus);

        unsigned m_subchannel_index = 0;
        std::vector<uint8_t> m_data;
};

// StreamSnoop is responsible for saving msc data into files,
// and calling DabPlusSnoop's decode routine if it's a DAB+ subchannel
class StreamSnoop {
    public:
        StreamSnoop(int subchid, bool dump_to_file) :
            dps(),
            m_subchid(subchid),
            m_raw_data_stream_fd(nullptr),
            m_dump_to_file(dump_to_file) {
                dps.subchid = subchid;
                dps.enable_wav_file_output(dump_to_file);
            }
        ~StreamSnoop();
        StreamSnoop(StreamSnoop&& other);

        StreamSnoop(const StreamSnoop& other) = delete;
        StreamSnoop& operator=(const StreamSnoop& other) = delete;

        void set_subchannel_index(unsigned subchannel_index)
        {
            dps.set_subchannel_index(subchannel_index);
        }

        void push(uint8_t* streamdata, size_t streamsize);

        audio_statistics_t get_audio_statistics(void) const;

        int stream_index = -1;

    private:
        DabPlusSnoop dps;
        int m_subchid = -1;
        FILE* m_raw_data_stream_fd;
        bool m_dump_to_file;
};

