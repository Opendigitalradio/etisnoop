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

bool fig0_18_is_complete(int services_id)
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


// FIG 0/18 Announcement support
// ETSI EN 300 401 8.1.6.1
fig_result_t fig0_18(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint32_t key;
    uint16_t SId, Asu_flags;
    uint8_t i = 1, j, Rfa, Number_clusters;
    fig_result_t r;
    uint8_t* f = fig0.f;

    while (i < (fig0.figlen - 4)) {
        // iterate over announcement support
        // SId, Asu flags, Rfa, Number of clusters
        SId = ((uint16_t)f[i] << 8) | (uint16_t)f[i+1];
        r.complete |= fig0_18_is_complete(SId);
        Asu_flags = ((uint16_t)f[i+2] << 8) | (uint16_t)f[i+3];
        Rfa = (f[i+4] >> 5);
        Number_clusters = (f[i+4] & 0x1F);
        r.msgs.emplace_back("-");
        r.msgs.emplace_back(1, strprintf("SId=0x%X", SId));
        r.msgs.emplace_back(1, strprintf("Asu flags=0x%04x", Asu_flags));
        if (Rfa != 0) {
            r.errors.push_back(strprintf("Rfa=%d invalid value", Rfa));
        }
        r.msgs.emplace_back(1, strprintf("Number of clusters=%d", Number_clusters));

        key = ((uint32_t)fig0.oe() << 17) | ((uint32_t)fig0.pd() << 16) | (uint32_t)SId;
        r.msgs.emplace_back(1, strprintf("database key=0x%05x", key));
        // CEI Change Event Indication
        if ((Number_clusters == 0) && (Asu_flags == 0)) {
            r.msgs.emplace_back("CEI=true");
        }
        i += 5;

        std::stringstream clusters_ss;
        for(j = 0; (j < Number_clusters) && (i < fig0.figlen); j++) {
            // iterate over Cluster Id
            if (j > 0) {
                clusters_ss << ", ";
            }
            clusters_ss << strprintf("0x%X", f[i]);
            i++;
        }
        r.msgs.emplace_back(1, "Cluster Ids: [" + clusters_ss.str() + "]");

        if (j < Number_clusters) {
            r.errors.push_back("missing Cluster Id, fig length too short !");
        }

        r.msgs.emplace_back(1, "Announcements:");
        // decode announcement support types
        for (j = 0; j < 16; j++) {
            if (Asu_flags & (1 << j)) {
                r.msgs.emplace_back(2, strprintf("- %s", get_announcement_type(j)));
            }
        }
    }

    return r;
}

