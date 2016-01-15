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

std::string get_fig_0_13_userapp(int user_app_type)
{
    switch (user_app_type) {
        case 0x000: return "Reserved for future definition";
        case 0x001: return "Not used";
        case 0x002: return "MOT Slideshow";
        case 0x003: return "MOT Broadacst Web Site";
        case 0x004: return "TPEG";
        case 0x005: return "DGPS";
        case 0x006: return "TMC";
        case 0x007: return "EPG";
        case 0x008: return "DAB Java";
        case 0x44a: return "Journaline";
        default: return "Reserved for future applications";
    }
}

// FIG 0/13 User application information
// ETSI EN 300 401 8.1.20
void fig0_13(fig0_common_t& fig0, int indent)
{
    uint32_t SId;
    uint8_t  SCIdS;
    uint8_t  No;
    uint8_t* f = fig0.f;
    const int figtype = 0;
    char desc[256];

    int k = 1;

    if (fig0.pd() == 0) { // Programme services, 16 bit SId
        SId  = (f[k] << 8) |
                f[k+1];
        k+=2;

        SCIdS = f[k] >> 4;
        No    = f[k] & 0x0F;
        k++;
    }
    else { // Data services, 32 bit SId
        SId   = (f[k]   << 24) |
            (f[k+1] << 16) |
            (f[k+2] << 8) |
            f[k+3];
        k+=4;

        SCIdS = f[k] >> 4;
        No    = f[k] & 0x0F;
        k++;

    }

    sprintf(desc, "FIG %d/%d: SId=0x%X SCIdS=%u No=%u",
            figtype, fig0.ext(), SId, SCIdS, No);
    printbuf(desc, indent+1, NULL, 0);

    for (int numapp = 0; numapp < No; numapp++) {
        uint16_t user_app_type = ((f[k] << 8) |
                (f[k+1] & 0xE0)) >> 5;
        uint8_t  user_app_len  = f[k+1] & 0x1F;
        k+=2;

        sprintf(desc, "User Application %d '%s'; length %u",
                user_app_type,
                get_fig_0_13_userapp(user_app_type).c_str(),
                user_app_len);
        printbuf(desc, indent+2, NULL, 0);
    }
}

