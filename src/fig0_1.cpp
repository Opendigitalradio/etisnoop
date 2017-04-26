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
#include <unordered_set>

static std::unordered_set<int> subchannels_seen;

bool fig0_1_is_complete(int subch_id)
{
    bool complete = subchannels_seen.count(subch_id);

    if (complete) {
        subchannels_seen.clear();
    }
    else {
        subchannels_seen.insert(subch_id);
    }

    return complete;
}

// FIG 0/1 Basic sub-channel organization
// ETSI EN 300 401 6.2.1
fig_result_t fig0_1(fig0_common_t& fig0, const display_settings_t &disp)
{
    int i = 1;
    uint8_t* f = fig0.f;
    fig_result_t r;

    while (i <= fig0.figlen-3) {
        // iterate over subchannels
        int subch_id = f[i] >> 2;
        r.complete |= fig0_1_is_complete(subch_id);

        int start_addr = ((f[i] & 0x03) << 8) |
            (f[i+1]);
        int long_flag  = (f[i+2] >> 7);

        if (long_flag) {
            int option = (f[i+2] >> 4) & 0x07;
            int protection_level = (f[i+2] >> 2) & 0x03;
            int subchannel_size  = ((f[i+2] & 0x03) << 8 ) |
                f[i+3];

            i += 4;

            r.msgs.push_back(strprintf("Subch 0x%x", subch_id));
            r.msgs.push_back(strprintf("start_addr %d", start_addr));
            r.msgs.emplace_back("long");

            if (option == 0x00) {
                r.msgs.push_back(strprintf("EEP %d-A", protection_level));
            }
            else if (option == 0x01) {
                r.msgs.push_back(strprintf("EEP %d-B", protection_level));
            }
            else {
                r.errors.push_back(strprintf("Invalid option %d protection %d", option, protection_level));
            }

            r.msgs.push_back(strprintf("subch size %d", subchannel_size));
        }
        else {
            int table_switch = (f[i+2] >> 6) & 0x01;
            uint32_t table_index  = (f[i+2] & 0x3F);

            r.msgs.push_back(strprintf("Subch 0x%x", subch_id));
            r.msgs.push_back(strprintf("start_addr %d", start_addr));
            r.msgs.emplace_back("short");
            if (table_switch != 0) {
                r.errors.push_back(strprintf("Invalid table_switch %d", table_switch));
            }
            r.msgs.push_back(strprintf("table index %d", table_index));

            i += 3;
        }
    }

    return r;
}

