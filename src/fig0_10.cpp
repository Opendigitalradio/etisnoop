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

    Authors:
         Sergio Sagliocco <sergio.sagliocco@csp.it>
         Matthias P. Braendli <matthias@mpb.li>
                   / |  |-  ')|)  |-|_ _ (|,_   .|  _  ,_ \
         Data Path \(|(||_(|/_| (||_||(a)_||||(|||.(_()|||/

*/

#include "figs.hpp"
#include <cstdio>
#include <cstring>
#include <map>

// FIG 0/10 Date and time
// ETSI EN 300 401 8.1.3.1
bool fig0_10(fig0_common_t& fig0, int indent)
{
    char desc[256];
    char dateStr[256];
    dateStr[0] = 0;
    uint8_t* f = fig0.f;

    //bool RFU = f[1] >> 7;

    uint32_t MJD = (((uint32_t)f[1] & 0x7F) << 10)    |
                   ((uint32_t)(f[2]) << 2) |
                   (f[3] >> 6);
    sprintfMJD(dateStr, MJD);

    bool LSI = f[3] & 0x20;
    bool ConfInd = f[3] & 0x10;
    fig0.wm_decoder.push_confind_bit(ConfInd);
    bool UTC = f[3] & 0x8;

    uint8_t hours = ((f[3] & 0x7) << 2) |
                    ( f[4] >> 6);

    uint8_t minutes = f[4] & 0x3f;

    if (UTC) {
        uint8_t seconds = f[5] >> 2;
        uint16_t milliseconds = ((uint16_t)(f[5] & 0x3) << 8) | f[6];

        sprintf(desc, "FIG 0/%d(long): MJD=0x%X %s, LSI %u, ConfInd %u, UTC Time: %02d:%02d:%02d.%d",
                fig0.ext(), MJD, dateStr, LSI, ConfInd, hours, minutes, seconds, milliseconds);
        printbuf(desc, indent+1, NULL, 0);
    }
    else {
        sprintf(desc, "FIG 0/%d(short): MJD=0x%X %s, LSI %u, ConfInd %u, UTC Time: %02d:%02d",
                fig0.ext(), MJD, dateStr, LSI, ConfInd, hours, minutes);
        printbuf(desc, indent+1, NULL, 0);
    }

    return true;
}

