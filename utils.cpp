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

#include "utils.hpp"
#include <cstring>

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

void printbuf(string header,
        int indent_level,
        uint8_t* buffer,
        size_t size,
        string desc)
{
    if (verbosity > 0) {
        for (int i = 0; i < indent_level; i++) {
            printf("\t");
        }

        printf("%s", header.c_str());

        if (verbosity > 1) {
            if (size != 0) {
                printf(": ");
            }

            for (size_t i = 0; i < size; i++) {
                printf("%02x ", buffer[i]);
            }
        }

        if (desc != "") {
            printf(" [%s] ", desc.c_str());
        }

        printf("\n");
    }
}

void printinfo(string header,
        int indent_level,
        int min_verb)
{
    if (verbosity >= min_verb) {
        for (int i = 0; i < indent_level; i++) {
            printf("\t");
        }
        printf("%s\n", header.c_str());
    }
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

char *strcatPNum(char *dest_str, uint16_t Programme_Number) {
    uint8_t day, hour, minute;
    char tempbuf[256];

    minute = (uint8_t)(Programme_Number & 0x003F);
    hour = (uint8_t)((Programme_Number >> 6) & 0x001F);
    day = (uint8_t)((Programme_Number >> 11) & 0x001F);
    if (day != 0) {
        sprintf(tempbuf, "day of month=%d time=%02d:%02d", day, hour, minute);
    }
    else {  // day == 0
        // Special codes are allowed when the date part of the PNum field
        // signals date = "0". In this case, the hours and minutes part of
        // the field shall contain a special code, as follows
        if ((hour == 0) && (minute == 0)) {
            sprintf(tempbuf, "Status code: no meaningful PNum is currently provided");
        }
        else if ((hour == 0) && (minute == 1)) {
            sprintf(tempbuf, "Blank code: the current programme is not worth recording");
        }
        else if ((hour == 0) && (minute == 2)) {
            sprintf(tempbuf, "Interrupt code: the interrupt is unplanned (for example a traffic announcement)");
        }
        else {
            sprintf(tempbuf, "invalid value");
        }
    }
    return strcat(dest_str, tempbuf);
}


