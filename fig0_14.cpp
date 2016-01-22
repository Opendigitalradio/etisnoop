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

static std::unordered_set<int> subch_ids_seen;

bool fig0_14_is_complete(int subch_id)
{
    bool complete = subch_ids_seen.count(subch_id);

    if (complete) {
        subch_ids_seen.clear();
    }
    else {
        subch_ids_seen.insert(subch_id);
    }

    return complete;
}


// fig 0/14 FEC Scheme: this 2-bit field shall indicate the Forward Error Correction scheme in use, as follows:
const char *FEC_schemes_str[4] =  {
    "no FEC scheme applied",
    "FEC scheme applied according to ETSI EN 300 401 clause 5.3.5",
    "reserved for future definition",
    "reserved for future definition"
};


// FIG 0/14 FEC sub-channel organization
// ETSI EN 300 401 6.2.2
bool fig0_14(fig0_common_t& fig0, int indent)
{
    uint8_t i = 1, SubChId, FEC_scheme;
    uint8_t* f = fig0.f;
    char desc[256];
    bool complete = false;

    while (i < fig0.figlen) {
        // iterate over Sub-channel
        SubChId = f[i] >> 2;
        complete |= fig0_14_is_complete(SubChId);
        FEC_scheme = f[i] & 0x3;
        sprintf(desc, "SubChId=0x%X, FEC scheme=%d %s",
                SubChId, FEC_scheme, FEC_schemes_str[FEC_scheme]);
        printbuf(desc, indent+1, NULL, 0);
        i++;
    }

    return complete;
}

