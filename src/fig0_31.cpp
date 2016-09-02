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
#include <unordered_set>

static std::unordered_set<uint64_t> figtype_flags_seen;

bool fig0_31_is_complete(uint64_t figtype_flags)
{
    bool complete = figtype_flags_seen.count(figtype_flags);

    if (complete) {
        figtype_flags_seen.clear();
    }
    else {
        figtype_flags_seen.insert(figtype_flags);
    }

    return complete;
}

// FIG 0/31 FIC re-direction
// ETSI EN 300 401 8.1.12
bool fig0_31(fig0_common_t& fig0, int indent)
{
    uint32_t FIG_type0_flag_field = 0, flag_field;
    uint8_t i = 1, j, FIG_type1_flag_field = 0, FIG_type2_flag_field = 0;
    char desc[256];
    uint8_t* f = fig0.f;
    bool complete = false;

    if (i < (fig0.figlen - 5)) {
        // Read FIC re-direction
        FIG_type0_flag_field = ((uint32_t)f[i] << 24) | ((uint32_t)f[i+1] << 16) |
            ((uint32_t)f[i+2] << 8) | (uint32_t)f[i+3];
        FIG_type1_flag_field = f[i+4];
        FIG_type2_flag_field = f[i+5];

        uint64_t key = ((uint64_t)FIG_type1_flag_field << 32) | ((uint64_t)FIG_type2_flag_field << 40) | FIG_type0_flag_field;
        complete |= fig0_31_is_complete(key);

        sprintf(desc, "FIG type 0 flag field=0x%X, FIG type 1 flag field=0x%X, FIG type 2 flag field=0x%X",
                FIG_type0_flag_field, FIG_type1_flag_field, FIG_type2_flag_field);
        printbuf(desc, indent+1, NULL, 0);

        for(j = 0; j < 32; j++) {
            // iterate over FIG type 0 re-direction
            flag_field = FIG_type0_flag_field & ((uint32_t)1 << j);
            if ((flag_field != 0) && ((j <= 5) || (j == 8) ||
                        (j == 10) || (j == 13) || (j == 14) ||
                        (j == 19) || (j == 26) || (j == 28))) {
                sprintf(desc, "fig0.oe()=%d FIG 0/%d carried in AIC, invalid configuration, shall always be carried entirely in the FIC",
                        fig0.oe(), j);
                printbuf(desc, indent+2, NULL, 0);
                fprintf(stderr, "WARNING: FIG 0/%d FIG re-direction of fig0.oe()=%d FIG0/%d not allowed\n", fig0.ext(), fig0.oe(), j);
            }
            else if ((flag_field != 0) && ((j == 21) || (j == 24))) {
                sprintf(desc, "fig0.oe()=%d FIG 0/%d carried in AIC, same shall be carried in FIC", fig0.oe(), j);
                printbuf(desc, indent+2, NULL, 0);
            }
            else if (flag_field != 0) {
                if (fig0.oe() == 0) {
                    sprintf(desc, "fig0.oe()=%d FIG 0/%d carried in AIC, same shall be carried in FIC", fig0.oe(), j);
                }
                else {  // fig0.oe() == 1
                    sprintf(desc, "fig0.oe()=%d FIG 0/%d carried in AIC, may be carried entirely in AIC", fig0.oe(), j);
                }
                printbuf(desc, indent+2, NULL, 0);
            }
        }

        for(j = 0; j < 8; j++) {
            // iterate over FIG type 1 re-direction
            flag_field = FIG_type1_flag_field & ((uint32_t)1 << j);
            if (flag_field != 0) {
                if (fig0.oe() == 0) {
                    sprintf(desc, "fig0.oe()=%d FIG 1/%d carried in AIC, same shall be carried in FIC", fig0.oe(), j);
                }
                else {  // fig0.oe() == 1
                    sprintf(desc, "fig0.oe()=%d FIG 1/%d carried in AIC, may be carried entirely in AIC", fig0.oe(), j);
                }
                printbuf(desc, indent+2, NULL, 0);
            }
        }

        for(j = 0; j < 8; j++) {
            // iterate over FIG type 2 re-direction
            flag_field = FIG_type2_flag_field & ((uint32_t)1 << j);
            if (flag_field != 0) {
                if (fig0.oe() == 0) {
                    sprintf(desc, "fig0.oe()=%d FIG 2/%d carried in AIC, same shall be carried in FIC", fig0.oe(), j);
                }
                else {  // fig0.oe() == 1
                    sprintf(desc, "fig0.oe()=%d FIG 2/%d carried in AIC, may be carried entirely in AIC", fig0.oe(), j);
                }
                printbuf(desc, indent+2, NULL, 0);
            }
        }
    }
    if (fig0.figlen != 7) {
        fprintf(stderr, "WARNING: FIG 0/%d invalid length %d, expecting 7\n", fig0.ext(), fig0.figlen);
    }

    return complete;
}

