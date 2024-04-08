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

static std::unordered_set<int> clusters_seen;

bool fig0_19_is_complete(int clusters_id)
{
    bool complete = clusters_seen.count(clusters_id);

    if (complete) {
        clusters_seen.clear();
    }

    clusters_seen.insert(clusters_id);

    return complete;
}

// FIG 0/19 Announcement switching
// ETSI EN 300 401 8.1.6.2
fig_result_t fig0_19(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint16_t Asw_flags;
    uint8_t i = 1, j, Cluster_Id, SubChId, Rfa, RegionId_LP;
    fig_result_t r;
    bool New_flag, Region_flag;
    uint8_t* f = fig0.f;

    while (i < (fig0.figlen - 3)) {
        // iterate over announcement switching
        // Cluster Id, Asw flags, New flag, Region flag,
        // SubChId, Rfa, Region Id Lower Part
        Cluster_Id = f[i];
        r.complete |= fig0_19_is_complete(Cluster_Id);
        Asw_flags = ((uint16_t)f[i+1] << 8) | (uint16_t)f[i+2];
        New_flag = (f[i+3] >> 7);
        Region_flag = (f[i+3] >> 6) & 0x1;
        SubChId = (f[i+3] & 0x3F);
        r.msgs.emplace_back("-");
        r.msgs.emplace_back(1, strprintf("Cluster Id=0x%02x", Cluster_Id));
        r.msgs.emplace_back(1, strprintf("Asw flags=0x%04x", Asw_flags));
        r.msgs.emplace_back(1, strprintf("New flag=%d %s", New_flag, (New_flag)?"new":"repeat"));
        r.msgs.emplace_back(1, strprintf("Region flag=%d last byte %s", Region_flag, (Region_flag)?"present":"absent"));
        r.msgs.emplace_back(1, strprintf("SubChId=%d", SubChId));
        if (Region_flag) {
            if (i < (fig0.figlen - 4)) {
                // read region lower part
                Rfa = (f[i+4] >> 6);
                RegionId_LP = (f[i+4] & 0x3F);
                if (Rfa != 0) {
                    r.errors.push_back(strprintf("Rfa=%d invalid value", Rfa));
                }
                r.msgs.emplace_back(1, strprintf("Region Lower Part=0x%02x", RegionId_LP));
            }
            else {
                r.errors.push_back("missing Region Lower Part, fig length too short !");
            }
        }
        // decode announcement switching types
        r.msgs.emplace_back(1, strprintf("Announcement switching:"));
        for(j = 0; j < 16; j++) {
            if (Asw_flags & (1 << j)) {
                r.msgs.emplace_back(2, strprintf("- %s", get_announcement_type(j)));
            }
        }
        i += (4 + Region_flag);
    }

    return r;
}

