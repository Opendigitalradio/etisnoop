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

    faad_decoder.h
        Use libfaad to decode the AAC content of the DAB+ subchannel

    Authors:
         Matthias P. Braendli <matthias@mpb.li>
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <vector>
#include <neaacdec.h>

#ifndef __FAAD_DECODER_H_
#define __FAAD_DECODER_H_

struct adts_fixed_header {
    unsigned int syncword           :12;
    unsigned int id                 :1;
    unsigned int layer              :2;
    unsigned int protection_absent  :1;
    unsigned int profile_objecttype :2;
    unsigned int sampling_freq_idx  :4;
    unsigned int private_bit        :1;
    unsigned int channel_conf       :3;
    unsigned int original_copy      :1;
    unsigned int home               :1;
};

struct adts_variable_header {
    unsigned int copyright_id_bit       :1;
    unsigned int copyright_id_start     :1;
    unsigned int aac_frame_length       :13;
    unsigned int adts_buffer_fullness   :11;
    unsigned int no_raw_data_blocks     :2;
};

class FaadHandle
{
    public:
        FaadHandle()
        {
            decoder = NeAACDecOpen();
        }

        FaadHandle(const FaadHandle& other)
        {
            this->decoder = NeAACDecOpen();
        }

        FaadHandle& operator=(const FaadHandle& other)
        {
            this->decoder = NeAACDecOpen();
            return *this;
        }

        ~FaadHandle()
        {
            NeAACDecClose(decoder);
            decoder = NULL;
        }

        NeAACDecHandle decoder;
};

class FaadDecoder
{
    public:
        FaadDecoder();

        void open(std::string filename, bool ps_flag, bool aac_channel_mode,
                bool dac_rate, bool sbr_flag, int mpeg_surround_config);

        void close(void);

        bool decode(std::vector<std::vector<uint8_t> > aus);

        bool is_initialised(void) { return m_initialised; }

    private:
        void update_header(void);
        size_t m_data_len;

        std::string m_filename;
        FILE* m_fd;
        FILE* m_aac;

        /* Data needed for FAAD */
        bool m_ps_flag;
        bool m_aac_channel_mode;
        bool m_dac_rate;
        bool m_sbr_flag;
        int  m_mpeg_surround_config;

        int  m_channels;
        int  m_sample_rate;

        bool m_initialised;
        FaadHandle m_faad_handle;
};

#endif

