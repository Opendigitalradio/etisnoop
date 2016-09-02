/*
   Copyright (C) 2015 Stefan Pöschel

   Copyright (C) 2015 Matthias P. Braendli (http://www.opendigitalradio.org)

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
   */

#include <stdexcept>
#include "rsdecoder.hpp"

RSDecoder::RSDecoder()
{
    rs_handle = init_rs_char(8, 0x11D, 0, 1, 10, 135);
    if(!rs_handle)
        throw std::runtime_error("RSDecoder: error while init_rs_char");
}

RSDecoder::~RSDecoder()
{
    free_rs_char(rs_handle);
}

int RSDecoder::DecodeSuperframe(std::vector<uint8_t> &sf, int subch_index)
{
    int total_corr_count = 0;
    bool uncorr_errors = false;

    std::vector<int> errors_per_index(subch_index);

    // process all RS packets
    for(int i = 0; i < subch_index; i++) {
        for(int pos = 0; pos < 120; pos++)
            rs_packet[pos] = sf[pos * subch_index + i];

        // detect errors
        int corr_count = decode_rs_char(rs_handle, rs_packet, corr_pos, 0);
        errors_per_index[i] = corr_count;

        if(corr_count == -1) {
            uncorr_errors = true;
        }
        else {
            total_corr_count += corr_count;
        }

        // correct errors
        for(int j = 0; j < corr_count; j++) {

            int pos = corr_pos[j] - 135;
            if(pos < 0)
                continue;

            //			fprintf(stderr, "j: %d, pos: %d, sf-index: %d\n", j, pos, pos * subch_index + i);
            sf[pos * subch_index + i] = rs_packet[pos];
        }
    }

    // output statistics
    if (total_corr_count || uncorr_errors) {
        printf("RS uncorrected errors:\n");
        for (size_t i = 0; i < errors_per_index.size(); i++) {
            int e = errors_per_index[i];
            printf(" (%zu: %d)", i, e);
        }
        printf("\n");
    }

    return uncorr_errors ? -1 : total_corr_count;
}


