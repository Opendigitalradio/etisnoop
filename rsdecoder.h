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

#include <stdint.h>
#include <vector>

extern "C" {
#include <fec.h>
}

class RSDecoder {
    private:
        void *rs_handle;
        uint8_t rs_packet[120];
        int corr_pos[10];
    public:
        RSDecoder();
        ~RSDecoder();

        /* Correct errors using reed-solomon decoder.
         * Returns number of errors corrected, or -1 if some errors could not
         * be corrected
         */
        int DecodeSuperframe(std::vector<uint8_t> &sf, int subch_index);
};


