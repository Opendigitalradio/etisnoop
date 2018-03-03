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

static std::unordered_set<int> components_seen;

bool fig0_5_is_complete(int components_id)
{
    bool complete = components_seen.count(components_id);

    if (complete) {
        components_seen.clear();
    }
    else {
        components_seen.insert(components_id);
    }

    return complete;
}

// FIG 0/5 Service component language
// ETSI EN 300 401 8.1.2
fig_result_t fig0_5(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint16_t SCId;
    uint8_t i = 1, SubChId, FIDCId, Language, Rfa;
    fig_result_t r;
    bool LS_flag, MSC_FIC_flag;

    uint8_t* f = fig0.f;

    while (i < fig0.figlen - 1) {
        // iterate over service component language
        LS_flag = f[i] >> 7;
        r.msgs.emplace_back("-");
        if (LS_flag == 0) {
            // Short form (L/S = 0)
            MSC_FIC_flag = (f[i] >> 6) & 0x01;
            Language = f[i+1];
            r.msgs.emplace_back(1, "form=short");
            r.msgs.emplace_back(1, strprintf("MSC/FIC flag=%d MSC", MSC_FIC_flag));

            if (MSC_FIC_flag == 0) {
                // 0: MSC in Stream mode and SubChId identifies the sub-channel
                SubChId = f[i] & 0x3F;
                r.msgs.emplace_back(1, strprintf("SubChId=0x%X", SubChId));
            }
            else {
                // 1: FIC and FIDCId identifies the component
                FIDCId = f[i] & 0x3F;
                r.msgs.emplace_back(1, strprintf("FIDCId=0x%X", FIDCId));
            }
            r.msgs.emplace_back(1, strprintf("Language=0x%X %s",
                        Language, get_language_name(Language)));

            int key = (MSC_FIC_flag << 7) | (f[i] % 0x3F);
            r.complete |= fig0_5_is_complete(key);
            i += 2;
        }
        else {
            // Long form (L/S = 1)
            if (i < (fig0.figlen - 2)) {
                r.msgs.emplace_back(1, "form=long");
                Rfa = (f[i] >> 4) & 0x07;

                SCId = (((uint16_t)f[i] & 0x0F) << 8) | (uint16_t)f[i+1];
                int key = (LS_flag << 15) | SCId;
                r.complete |= fig0_5_is_complete(key);
                Language = f[i+2];
                if (Rfa != 0) {
                    r.errors.emplace_back(strprintf("Rfa=%d invalid value", Rfa));
                }

                r.msgs.emplace_back(1, strprintf("SCId=0x%X", SCId));
                r.msgs.emplace_back(1, strprintf("Language=0x%X %s",
                            Language, get_language_name(Language)));
            }
            else {
                r.errors.emplace_back("Long form FIG is too short");
            }
            i += 3;
        }
    }

    return r;
}

