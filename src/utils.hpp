/*
    Copyright (C) 2014 CSP Innovazione nelle ICT s.c.a r.l. (http://www.csp.it/)
    Copyright (C) 2015 Data Path
    Copyright (C) 2025 Matthias P. Braendli (http://www.opendigitalradio.org)

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

void set_verbosity(int v);
int  get_verbosity(void);

std::string strprintf(const char* fmt, ...);

struct display_settings_t {
    display_settings_t(bool print, int indent) :
        _print(print), _indent(indent) {}

    display_settings_t indent() const {
        return display_settings_t(_print, _indent+1);
    }

    int get_indent() const { return _indent; }

    void set_print(bool p) { _print = p; }

    void printsequencestart();

    void printinfo(const std::string &header,
            int min_verb);

    void printvalue(const std::string& header,
            const std::string& desc="",
            const std::string& value="");

    void printfig(const std::string& header,
            uint8_t* buffer = nullptr,
            size_t size = 0,
            const std::string& desc="",
            const std::string& value="");

    void printbuf(const std::string& header,
            uint8_t* buffer = nullptr,
            size_t size = 0,
            const std::string& desc="",
            const std::string& value="");

    private:
    bool _print;
    int _indent;

    void printyaml(const std::string& header,
            uint8_t* buffer = nullptr,
            size_t size = 0,
            const std::string& desc = "",
            const std::string& value = "");
};

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
