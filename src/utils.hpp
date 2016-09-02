/*
    Copyright (C) 2014 CSP Innovazione nelle ICT s.c.a r.l. (http://www.csp.it/)
    Copyright (C) 2016 Matthias P. Braendli (http://www.opendigitalradio.org)
    Copyright (C) 2015 Data Path

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

    etisnoop.cpp
          Parse ETI NI G.703 file

    Authors:
         Sergio Sagliocco <sergio.sagliocco@csp.it>
         Matthias P. Braendli <matthias@mpb.li>
                   / |  |-  ')|)  |-|_ _ (|,_   .|  _  ,_ \
         Data Path \(|(||_(|/_| (||_||(a)_||||(|||.(_()|||/

*/

#pragma once

#include <string>
#include <cstdint>
#include <cinttypes>

void set_verbosity(int v);
int  get_verbosity(void);

void printbuf(std::string header,
        int indent_level,
        uint8_t* buffer,
        size_t size,
        std::string desc="");

void printinfo(std::string header,
        int indent_level,
        int min_verb);

// sprintfMJD: convert MJD (Modified Julian Date) into date string
int sprintfMJD(char *dst, int mjd);

// strcatPNum decode Programme_Number into string and append it to dest_str
// Programme_Number: this 16-bit field shall define the date and time at which
// a programme begins or will be continued. This field is coded in the same way
// as the RDS "Programme Item Number (PIN)" feature (EN 62106).
char *strcatPNum(char *dest_str, uint16_t Programme_Number);

