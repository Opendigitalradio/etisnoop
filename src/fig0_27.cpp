/*
    Copyright (C) 2014 CSP Innovazione nelle ICT s.c.a r.l. (http://www.csp.it/)
    Copyright (C) 2017 Matthias P. Braendli (http://www.opendigitalradio.org)
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
fig_result_t fig0_27(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint16_t SId, PI;
    uint8_t i = 1, j, Rfu, Number_PI_codes, key;
    fig_result_t r;
    uint8_t* f = fig0.f;

    while (i < (fig0.figlen - 2)) {
        // iterate over FM announcement support
        SId = ((uint16_t)f[i] << 8) | (uint16_t)f[i+1];
        r.complete |= fig0_27_is_complete(SId);
        Rfu = f[i+2] >> 4;
        Number_PI_codes = f[i+2] & 0x0F;
        key = (fig0.oe() << 5) | (fig0.pd() << 4) | Number_PI_codes;
        r.msgs.push_back(strprintf("SId=0x%X", SId));
        if (Rfu != 0) {
            r.errors.push_back(strprintf("Rfu=%d invalid value", Rfu));
        }
        r.msgs.emplace_back(1, strprintf("Number of PI codes=%d", Number_PI_codes));
        if (Number_PI_codes > 12) {
            r.errors.push_back(strprintf("Number of PI codes=%d > 12 (maximum value)", Number_PI_codes));
        }
        r.msgs.emplace_back(1, strprintf("database key=0x%02X", key));
        // CEI Change Event Indication
        if (Number_PI_codes == 0) {
            // The Change Event Indication (CEI) is signalled by the Number of PI codes field = 0
            r.msgs.emplace_back(1, "CEI");
        }
        i += 3;

        for (j = 0; j < Number_PI_codes && i < fig0.figlen - 1; j++) {
            // iterate over PI
            PI = ((uint16_t)f[i] << 8) | (uint16_t)f[i+1];
            r.msgs.emplace_back(2, strprintf("PI=0x%X", PI));
            i += 2;
        }
        if (j != Number_PI_codes) {
            r.errors.push_back("fig length too short !");
        }
    }

    return r;
}

