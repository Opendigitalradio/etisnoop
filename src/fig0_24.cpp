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

bool fig0_24_is_complete(int services_id)
{
    bool complete = services_seen.count(services_id);

    if (complete) {
        services_seen.clear();
    }

    services_seen.insert(services_id);

    return complete;
}

// FIG 0/24 fig0.oe() Services
// ETSI EN 300 401 8.1.10.2
fig_result_t fig0_24(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint64_t key;
    uint32_t SId;
    uint16_t EId;
    uint8_t i = 1, j, Number_of_EIds, CAId;
    fig_result_t r;
    uint8_t* f = fig0.f;
    bool Rfa;

    while (i < (fig0.figlen - (((uint8_t)fig0.pd() + 1) * 2))) {
        // iterate over other ensembles services
        if (fig0.pd() == 0) {
            SId = ((uint32_t)f[i] << 8) | (uint32_t)f[i+1];
            i += 2;
        }
        else {  // fig0.pd() == 1
            SId = ((uint32_t)f[i] << 24) | ((uint32_t)f[i+1] << 16) |
                ((uint32_t)f[i+2] << 8) | (uint32_t)f[i+3];
            i += 4;
        }
        r.complete |= fig0_24_is_complete(SId);
        Rfa  =  (f[i] >> 7);
        CAId  = (f[i] >> 4);
        Number_of_EIds  = (f[i] & 0x0f);
        key = ((uint64_t)fig0.oe() << 33) | ((uint64_t)fig0.pd() << 32) | \
              (uint64_t)SId;

        r.msgs.emplace_back("-");
        r.msgs.emplace_back(1, strprintf("PD=%d", fig0.pd()));
        r.msgs.emplace_back(1, strprintf("SId=0x%X", SId));
        r.msgs.emplace_back(1, strprintf("CAId=%d", CAId));
        r.msgs.emplace_back(1, strprintf("Number of EId=%d", Number_of_EIds));
        r.msgs.emplace_back(1, strprintf("database key=%09" PRId64, key));

        if (Rfa != 0) {
            r.errors.push_back(strprintf("Rfa=%d invalid value", Rfa));
        }

        // CEI Change Event Indication
        if (Number_of_EIds == 0) {
            r.msgs.emplace_back("CEI=true");
        }
        i++;

        std::stringstream eid_ss;
        for (j = i; ((j < (i + (Number_of_EIds * 2))) && (j < fig0.figlen)); j += 2) {
            // iterate over EIds
            EId = ((uint16_t)f[j] <<8) | (uint16_t)f[j+1];
            if (j > i) {
                eid_ss << ", ";
            }
            eid_ss << strprintf("0x%04x", EId);
        }
        r.msgs.emplace_back(1, "EIds: [" + eid_ss.str() + "]");

        i += (Number_of_EIds * 2);
    }

    return r;
}

