/*
    Copyright (C) 2014 CSP Innovazione nelle ICT s.c.a r.l. (http://www.csp.it/)
    Copyright (C) 2018 Matthias P. Braendli (http://www.opendigitalradio.org)
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

struct display_settings_t {
    display_settings_t(bool _print, int _indent) :
        print(_print), indent(_indent) {}

    display_settings_t operator+(int indent_offset) const;

    bool print;
    int indent;
};

std::string strprintf(const char* fmt, ...);

void printfig(const std::string& header,
        const display_settings_t &disp,
        uint8_t* buffer,
        size_t size,
        const std::string& desc="",
        const std::string& value="");

void printbuf(const std::string& header,
        int indent,
        uint8_t* buffer=nullptr,
        size_t size=0,
        const std::string& desc="",
        const std::string& value="");

void printbuf(const std::string& header,
        const display_settings_t &disp,
        uint8_t* buffer,
        size_t size,
        const std::string& desc="",
        const std::string& value="");


void printvalue(const std::string& header,
        int indent = 0,
        const std::string& desc="",
        const std::string& value="");

void printvalue(const std::string& header,
        const display_settings_t &disp,
        const std::string& desc="",
        const std::string& value="");

void printinfo(const std::string &header,
        const display_settings_t &disp,
        int min_verb);

void printinfo(const std::string &header,
        int min_verb);

void printsequencestart(int indent = 0);

// sprintfMJD: convert MJD (Modified Julian Date) into date string
int sprintfMJD(char *dst, int mjd);

// Convert Programme Number to string
// Programme_Number: this 16-bit field shall define the date and time at which
// a programme begins or will be continued. This field is coded in the same way
// as the RDS "Programme Item Number (PIN)" feature (EN 62106).
std::string pnum_to_str(uint16_t Programme_Number);

// Convert and absolute 16-bit int value to dB. If value is zero, returns
// -90dB
int absolute_to_dB(int16_t value);

uint32_t read_u32_from_buf(const uint8_t *buf);
uint16_t read_u16_from_buf(const uint8_t *buf);
