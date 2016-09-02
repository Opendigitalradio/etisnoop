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
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <time.h>
#include "utils.hpp"


static uint8_t Mode_Identity = 0;

void set_mode_identity(uint8_t mid)
{
    Mode_Identity = mid;
}

uint8_t get_mode_identity()
{
    return Mode_Identity;
}

static size_t International_Table_Id;

size_t get_international_table(void)
{
    return International_Table_Id;
}

void   set_international_table(size_t intl_table)
{
    International_Table_Id = intl_table;
}


void figs_cleardb()
{
    // remove elements from fig0_6_key_la and fig0_22_key_Lat_Lng map containers
    fig0_22_cleardb();
    fig0_6_cleardb();
}


bool fig0_select(fig0_common_t& fig0, int indent)
{
    switch (fig0.ext()) {
        case 0: return fig0_0(fig0, indent); break;
        case 1: return fig0_1(fig0, indent); break;
        case 2: return fig0_2(fig0, indent); break;
        case 3: return fig0_3(fig0, indent); break;
        case 5: return fig0_5(fig0, indent); break;
        case 6: return fig0_6(fig0, indent); break;
        case 8: return fig0_8(fig0, indent); break;
        case 9: return fig0_9(fig0, indent); break;
        case 10: return fig0_10(fig0, indent); break;
        case 11: return fig0_11(fig0, indent); break;
        case 13: return fig0_13(fig0, indent); break;
        case 14: return fig0_14(fig0, indent); break;
        case 16: return fig0_16(fig0, indent); break;
        case 17: return fig0_17(fig0, indent); break;
        case 18: return fig0_18(fig0, indent); break;
        case 19: return fig0_19(fig0, indent); break;
        case 21: return fig0_21(fig0, indent); break;
        case 22: return fig0_22(fig0, indent); break;
        case 24: return fig0_24(fig0, indent); break;
        case 25: return fig0_25(fig0, indent); break;
        case 26: return fig0_26(fig0, indent); break;
        case 27: return fig0_27(fig0, indent); break;
        case 28: return fig0_28(fig0, indent); break;
        case 31: return fig0_31(fig0, indent); break;
        default: {
                     char desc[256];
                     sprintf(desc, "FIG 0/%d: unknown", fig0.ext());
                     printbuf(desc, indent, fig0.f+1, fig0.figlen-1);
                     break;
                 }
    }

    return false;
}

