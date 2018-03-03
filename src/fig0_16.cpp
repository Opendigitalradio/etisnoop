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
#include <set>

/* SId and PNum look like good candidates to uniquely identify a FIG0_16
 */
using SId_t = int;
using PNum_t = int;
static std::set<std::pair<SId_t, PNum_t> > components_seen;

bool fig0_16_is_complete(SId_t SId, PNum_t PNum)
{
    auto key = std::make_pair(SId, PNum);
    bool complete = components_seen.count(key);

    if (complete) {
        components_seen.clear();
    }
    else {
        components_seen.insert(key);
    }

    return complete;
}


// FIG 0/16 Programme Number & fig0.oe() Programme Number
// ETSI EN 300 401 8.1.4 & 8.1.10.3
fig_result_t fig0_16(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint16_t SId, PNum, New_SId, New_PNum;
    uint8_t i = 1, Rfa, Rfu;
    fig_result_t r;
    bool Continuation_flag, Update_flag;
    uint8_t* f = fig0.f;

    while (i < (fig0.figlen - 4)) {
        // iterate over Programme Number
        SId = ((uint16_t)f[i] << 8) | ((uint16_t)f[i+1]);
        PNum = ((uint16_t)f[i+2] << 8) | ((uint16_t)f[i+3]);
        r.complete |= fig0_16_is_complete(SId, PNum);
        Rfa = f[i+4] >> 6;
        Rfu = (f[i+4] >> 2) & 0x0F;
        Continuation_flag = (f[i+4] >> 1) & 0x01;
        Update_flag = f[i+4] & 0x01;

        r.msgs.emplace_back("-");
        r.msgs.emplace_back(1, strprintf("SId=0x%X", SId));
        r.msgs.emplace_back(1, strprintf("PNum=0x%X ", PNum) + pnum_to_str(PNum));

        if (Rfa != 0) {
            r.errors.push_back(strprintf("Rfa=%d invalid value", Rfa));
        }

        if (Rfu != 0) {
            r.errors.push_back(strprintf(", Rfu=0x%X invalid value", Rfu));
        }

        r.msgs.emplace_back(1, strprintf("Continuation flag=%d, the programme will %s",
                Continuation_flag,
                Continuation_flag ? "be interrupted but continued later" : "not be subject to a planned interruption"));
        r.msgs.emplace_back(1, strprintf("Update flag=%d %sre-direction",
                Update_flag, Update_flag ? "" : "no "));
        i += 5;

        if (Update_flag != 0) {
            // In the case of a re-direction, the New SId and New PNum shall be appended
            if (i < (fig0.figlen - 1)) {
                New_SId = ((uint16_t)f[i] << 8) | ((uint16_t)f[i+1]);
                r.msgs.emplace_back(1, strprintf("New SId=0x%X", New_SId));
                if (i < (fig0.figlen - 3)) {
                    New_PNum = ((uint16_t)f[i+2] << 8) | ((uint16_t)f[i+3]);
                    r.msgs.emplace_back(1, strprintf("New PNum=0x%X ", New_PNum) + pnum_to_str(New_PNum));
                }
                else {
                    r.errors.push_back("missing New PNum !");
                }
            }
            else {
                r.errors.push_back("missing New SId and New PNum !");
            }
            i += 4;
        }
    }

    return r;
}

