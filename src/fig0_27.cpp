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

static std::unordered_set<int> services_seen;

bool fig0_27_is_complete(int services_id)
{
    bool complete = services_seen.count(services_id);

    if (complete) {
        services_seen.clear();
    }
    else {
        services_seen.insert(services_id);
    }

    return complete;
}


// FIG 0/27 FM Announcement support
// ETSI EN 300 401 8.1.11.2.1
bool fig0_27(fig0_common_t& fig0, int indent)
{
    uint16_t SId, PI;
    uint8_t i = 1, j, Rfu, Number_PI_codes, key;
    char tmpbuf[256];
    char desc[256];
    uint8_t* f = fig0.f;
    bool complete = false;

    while (i < (fig0.figlen - 2)) {
        // iterate over FM announcement support
        SId = ((uint16_t)f[i] << 8) | (uint16_t)f[i+1];
        complete |= fig0_27_is_complete(SId);
        Rfu = f[i+2] >> 4;
        Number_PI_codes = f[i+2] & 0x0F;
        key = (fig0.oe() << 5) | (fig0.pd() << 4) | Number_PI_codes;
        sprintf(desc, "SId=0x%X", SId);
        if (Rfu != 0) {
            sprintf(tmpbuf, ", Rfu=%d invalid value", Rfu);
            strcat(desc, tmpbuf);
        }
        sprintf(tmpbuf, ", Number of PI codes=%d", Number_PI_codes);
        strcat(desc, tmpbuf);
        if (Number_PI_codes > 12) {
            strcat(desc, " above maximum value of 12");
            fprintf(stderr, "WARNING: FIG 0/%d Number of PI codes=%d > 12 (maximum value)\n", fig0.ext(), Number_PI_codes);
        }
        sprintf(tmpbuf, ", database key=0x%02X", key);
        strcat(desc, tmpbuf);
        // CEI Change Event Indication
        if (Number_PI_codes == 0) {
            // The Change Event Indication (CEI) is signalled by the Number of PI codes field = 0
            strcat(desc, ", CEI");
        }
        printbuf(desc, indent+1, NULL, 0);
        i += 3;
        for(j = 0; (j < Number_PI_codes) && (i < (fig0.figlen - 1)); j++) {
            // iterate over PI
            PI = ((uint16_t)f[i] << 8) | (uint16_t)f[i+1];
            sprintf(desc, "PI=0x%X", PI);
            printbuf(desc, indent+2, NULL, 0);
            i += 2;
        }
        if (j != Number_PI_codes) {
            sprintf(desc, "fig length too short !");
            printbuf(desc, indent+2, NULL, 0);
            fprintf(stderr, "WARNING: FIG 0/%d length %d too short !\n", fig0.ext(), fig0.figlen);
        }
    }

    return complete;
}

