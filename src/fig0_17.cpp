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

static std::unordered_set<int> services_ids_seen;

bool fig0_17_is_complete(int services_id)
{
    bool complete = services_ids_seen.count(services_id);

    if (complete) {
        services_ids_seen.clear();
    }

    services_ids_seen.insert(services_id);

    return complete;
}

// FIG 0/17 Programme Type
// ETSI EN 300 401 8.1.5
fig_result_t fig0_17(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint16_t SId;
    uint8_t i = 1, Rfa, Language, Int_code, Comp_code;
    fig_result_t r;
    bool SD_flag, PS_flag, L_flag, CC_flag, Rfu;
    uint8_t* f = fig0.f;

    while (i < (fig0.figlen - 3)) {
        // iterate over announcement support
        SId = (f[i] << 8) | f[i+1];
        r.complete |= fig0_17_is_complete(SId);
        SD_flag = (f[i+2] >> 7);
        PS_flag = ((f[i+2] >> 6) & 0x01);
        L_flag = ((f[i+2] >> 5) & 0x01);
        CC_flag = ((f[i+2] >> 4) & 0x01);
        Rfa = (f[i+2] & 0x0F);
        r.msgs.emplace_back("-");
        r.msgs.emplace_back(1, strprintf("SId=0x%X", SId));
        r.msgs.emplace_back(1, strprintf(
                    "S/D=%d Programme Type codes and language (when present), %srepresent the current programme contents",
                    SD_flag, SD_flag?"":"may not "));
        r.msgs.emplace_back(1, strprintf("P/S=%d %s service component",
                    PS_flag, PS_flag?"secondary":"primary"));
        r.msgs.emplace_back(1, strprintf("L flag=%d language field %s",
                L_flag, L_flag?"present":"absent"));
        r.msgs.emplace_back(1, strprintf("CC flag=%d complementary code and preceding Rfa and Rfu fields %s",
                CC_flag, CC_flag?"present":"absent"));

        if (Rfa != 0) {
            r.errors.push_back(strprintf("Rfa=0x%X invalid value", Rfa));
        }

        i += 3;
        if (L_flag != 0) {
            if (i < fig0.figlen) {
                Language = f[i];
                r.msgs.emplace_back(1, strprintf("Language=0x%X %s", Language,
                        get_language_name(Language)));
            }
            else {
                r.errors.push_back(strprintf("Language= invalid FIG length"));
            }
            i++;
        }

        if (i < fig0.figlen) {
            Rfa = f[i] >> 6;
            Rfu = (f[i] >> 5) & 0x01;
            if (Rfa != 0) {
                r.errors.push_back(strprintf("Rfa=0x%X invalid value", Rfa));
            }
            if (Rfu != 0) {
                r.errors.push_back(strprintf("Rfu=%d invalid value", Rfu));
            }
            Int_code = f[i] & 0x1F;
            r.msgs.emplace_back(1, strprintf("Int code=0x%X %s", Int_code,
                        get_programme_type(get_international_table(), Int_code)));
            i++;
        }
        else {
            r.errors.push_back("Int code: invalid FIG length");
        }

        if (CC_flag != 0) {
            if (i < fig0.figlen) {
                Rfa = f[i] >> 6;
                Rfu = (f[i] >> 5) & 0x01;
                if (Rfa != 0) {
                    r.errors.push_back(strprintf("Rfa=0x%X invalid value", Rfa));
                }
                if (Rfu != 0) {
                    r.errors.push_back(strprintf("Rfu=%d invalid value", Rfu));
                }
                Comp_code = f[i] & 0x1F;
                r.msgs.emplace_back(1, strprintf("Comp code=0x%X %s", Comp_code,
                            get_programme_type(get_international_table(), Comp_code)));
                i++;
            }
            else {
                r.errors.push_back(strprintf("Comp code= invalid FIG length"));
            }
        }
    }

    return r;
}

