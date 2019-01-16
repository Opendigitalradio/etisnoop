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

#include "utils.hpp"
#include <cstring>
#include <sstream>
#include <cmath>
#include <limits>
#include <stdarg.h>

using namespace std;

static int verbosity = 0;

void set_verbosity(int v)
{
    verbosity = v;
}

int get_verbosity()
{
    return verbosity;
}

display_settings_t display_settings_t::operator+(int indent_offset) const
{
    return display_settings_t(print, indent+indent_offset);
}

std::string strprintf(const char* fmt, ...)
{
    int size = 512;
    std::string str;
    va_list ap;
    while (1) {
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.c_str(), size, fmt, ap);
        va_end(ap);
        if (n > -1 && n < size) {
            str.resize(n);
            break;
        }
        if (n > -1)
            size = n + 1;
        else
            size *= 2;
    }

    return str;
}

static void printyaml(const string& header,
        const display_settings_t &disp,
        uint8_t* buffer = nullptr,
        size_t size = 0,
        const std::string& desc = "",
        const std::string& value = "")
{
    if (disp.print) {
        stringstream ss;
        for (int i = 0; i < disp.indent; i++) {
            ss << " ";
        }

        ss << header;
        ss << ":";

        if (not value.empty() and desc.empty() and not buffer) {
            ss << " " << value;
        }
        else {
            if (not value.empty()) {
                ss << "\n";
                for (int i = 0; i < disp.indent + 1; i++) {
                    ss << " ";
                }
                ss << strprintf("value: %s", value.c_str());
            }

            if (not desc.empty()) {
                ss << "\n";
                for (int i = 0; i < disp.indent + 1; i++) {
                    ss << " ";
                }
                ss << strprintf("desc: %s", desc.c_str());
            }

            if (buffer and verbosity > 0) {
                if (size != 0) {
                    ss << "\n";
                    for (int i = 0; i < disp.indent + 1; i++) {
                        ss << " ";
                    }
                    ss << "data: [";

                    size_t num_printed = 0;

                    for (size_t i = 0; i < size; i++) {
                        if (i > 0) {
                            ss << ",";
                            num_printed++;
                        }

                        if (num_printed + disp.indent + 1 + 7 > 60 ) {
                            ss << "\n";
                            for (int i = 0; i < disp.indent + 8; i++) {
                                ss << " ";
                            }
                            num_printed = 2;
                        }
                        else if (i > 0) {
                            ss << " ";
                            num_printed++;
                        }

                        ss << strprintf("0x%02x", buffer[i]);
                        num_printed += 3;
                    }
                    ss << "]";
                }
            }
        }

        ss << "\n";
        printf("%s", ss.str().c_str());
    }
}

void printbuf(const std::string& header,
        int indent,
        uint8_t* buffer,
        size_t size,
        const std::string& desc,
        const std::string& value)
{
    display_settings_t disp(true, indent);
    printyaml(header, disp, buffer, size, desc, value);
}

void printbuf(const string& header,
        const display_settings_t &disp,
        uint8_t* buffer,
        size_t size,
        const std::string& desc,
        const std::string& value)
{
    display_settings_t d = disp;
    d.print = (verbosity > 0);
    printyaml(header, d, buffer, size, desc, value);
}

void printbuf(const std::string& header, int indent)
{
    display_settings_t disp(true, indent);
    return printyaml(header, disp);
}

void printfig(const string& header,
        const display_settings_t &disp,
        uint8_t* buffer,
        size_t size,
        const std::string& desc,
        const std::string& value)
{
    printyaml(header, disp, buffer, size, desc, value);
}

void printvalue(const std::string& header,
        const display_settings_t &disp,
        const std::string& desc,
        const std::string& value)
{
    return printyaml(header, disp, nullptr, 0, desc, value);
}

void printvalue(const std::string& header,
        int indent,
        const std::string& desc,
        const std::string& value)
{
    display_settings_t disp(true, indent);
    return printyaml(header, disp, nullptr, 0, desc, value);
}

void printinfo(const string &header,
        const display_settings_t &disp,
        int min_verb)
{
    if (verbosity >= min_verb) {
        for (int i = 0; i < disp.indent; i++) {
            printf(" ");
        }
        printf("info: %s\n", header.c_str());
    }
}

void printinfo(const std::string &header, int min_verb)
{
    const display_settings_t disp(true, 0);
    printinfo(header, min_verb);
}

void printsequencestart(int indent)
{
    for (int i = 0; i < indent; i++) {
        printf(" ");
    }
    printf("-\n");
}

int sprintfMJD(char *dst, int mjd) {
    // EN 62106 Annex G
    // These formulas are applicable between the inclusive dates: 1st March 1900 to 28th February 2100
    int y, m, k;
    struct tm timeDate;

    memset(&timeDate, 0, sizeof(struct tm));

    // find Y, M, D from MJD
    y = (int)(((double)mjd - 15078.2) / 365.25);
    m = (int)(((double)mjd - 14956.1 - (int)((double)y * 365.25)) / 30.6001);
    timeDate.tm_mday = mjd - 14956 - (int)((double)y * 365.25) - (int)((double)m * 30.6001);
    if ((m == 14) || (m == 15)) {
        k = 1;
    }
    else {
        k = 0;
    }
    timeDate.tm_year = y + k;
    timeDate.tm_mon = (m - 1 - (k * 12)) - 1;

    // find WD from MJD
    timeDate.tm_wday = (((mjd + 2) % 7) + 1) % 7;

    //timeDate.tm_yday = 0; // Number of days since the first day of January not calculated
    timeDate.tm_isdst = -1; // No time print then information not available

    // print date string
    if ((timeDate.tm_mday < 0) || (timeDate.tm_mon < 0) || (timeDate.tm_year < 0)) {
        return sprintf(dst, "invalid MJD mday=%d mon=%d year=%d", timeDate.tm_mday, timeDate.tm_mon, timeDate.tm_year);
    }
    return strftime(dst, 256, "%a %b %d %Y", &timeDate);
}

std::string pnum_to_str(uint16_t Programme_Number)
{
    uint8_t day, hour, minute;

    minute = (uint8_t)(Programme_Number & 0x003F);
    hour = (uint8_t)((Programme_Number >> 6) & 0x001F);
    day = (uint8_t)((Programme_Number >> 11) & 0x001F);
    if (day != 0) {
        return strprintf("day of month=%d time=%02d:%02d", day, hour, minute);
    }
    else {  // day == 0
        // Special codes are allowed when the date part of the PNum field
        // signals date = "0". In this case, the hours and minutes part of
        // the field shall contain a special code, as follows
        if ((hour == 0) && (minute == 0)) {
            return "Status code: no meaningful PNum is currently provided";
        }
        else if ((hour == 0) && (minute == 1)) {
            return "Blank code: the current programme is not worth recording";
        }
        else if ((hour == 0) && (minute == 2)) {
            return "Interrupt code: the interrupt is unplanned "
                "(for example a traffic announcement)";
        }
        else {
            return "invalid value";
        }
    }
}

int absolute_to_dB(int16_t value)
{
    const int16_t int16_max = std::numeric_limits<int16_t>::max();
    return value ? round(20*log10((double)value / int16_max)) : -90;
}

