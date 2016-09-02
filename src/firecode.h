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

#ifndef _FIRECODE_H_
#define _FIRECODE_H_

#include <stdint.h>
#include <stdlib.h>

uint16_t firecode_crc(uint8_t* buf, size_t size);

#endif

