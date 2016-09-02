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

    firecode.c
         Implement a FireCode CRC calculator

    Authors:
         Matthias P. Braendli <matthias@mpb.li>
*/

#include "firecode.h"

uint16_t firecode_crc(uint8_t* buf, size_t size)
{
    int crc;
    int gen_poly;

    crc = 0x0000;
    gen_poly = 0x782F;	// 0111 1000 0010 1111 (16, 14, 13, 12, 11, 5, 3, 2, 1, 0)

    for (size_t len = 0; len < size; len++) {
        for (int i = 0x80; i != 0; i >>= 1) {
            if ((crc & 0x8000) != 0) {
                crc = (crc << 1) ^ gen_poly;
            }
            else {
                crc = crc << 1;
            }
            if ((buf[len] & i) != 0) {
                crc ^= gen_poly;
            }
        }
    }

    return crc & 0xFFFF;
}

