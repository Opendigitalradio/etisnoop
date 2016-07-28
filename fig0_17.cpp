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

static std::unordered_set<int> services_ids_seen;

bool fig0_17_is_complete(int services_id)
{
    bool complete = services_ids_seen.count(services_id);

    if (complete) {
        services_ids_seen.clear();
    }
    else {
        services_ids_seen.insert(services_id);
    }

    return complete;
}

// FIG 0/17 Programme Type
// ETSI EN 300 401 8.1.5
bool fig0_17(fig0_common_t& fig0, int indent)
{
    uint16_t SId;
    uint8_t i = 1, Rfa, Language, Int_code, Comp_code;
    char tmpbuf[512];
    char desc[512];
    bool SD_flag, PS_flag, L_flag, CC_flag, Rfu;
    uint8_t* f = fig0.f;
    bool complete = false;

    while (i < (fig0.figlen - 3)) {
        // iterate over announcement support
        SId = (f[i] << 8) | f[i+1];
        complete |= fig0_17_is_complete(SId);
        SD_flag = (f[i+2] >> 7);
        PS_flag = ((f[i+2] >> 6) & 0x01);
        L_flag = ((f[i+2] >> 5) & 0x01);
        CC_flag = ((f[i+2] >> 4) & 0x01);
        Rfa = (f[i+2] & 0x0F);
        sprintf(desc, "SId=0x%X, S/D=%d Programme Type codes and language (when present), %srepresent the current programme contents, P/S=%d %s service component, L flag=%d language field %s, CC flag=%d complementary code and preceding Rfa and Rfu fields %s",
                SId, SD_flag, SD_flag?"":"may not ", PS_flag, PS_flag?"secondary":"primary",
                L_flag, L_flag?"present":"absent", CC_flag, CC_flag?"present":"absent");
        if (Rfa != 0) {
            sprintf(tmpbuf, ", Rfa=0x%X invalid value", Rfa);
            strcat(desc, tmpbuf);
        }
        i += 3;
        if (L_flag != 0) {
            if (i < fig0.figlen) {
                Language = f[i];
                sprintf(tmpbuf, ", Language=0x%X %s", Language,
                        get_language_name(Language));
                strcat(desc, tmpbuf);
            }
            else {
                sprintf(tmpbuf, ", Language= invalid FIG length");
                strcat(desc, tmpbuf);
            }
            i++;
        }
        if (i < fig0.figlen) {
            Rfa = f[i] >> 6;
            Rfu = (f[i] >> 5) & 0x01;
            if (Rfa != 0) {
                sprintf(tmpbuf, ", Rfa=0x%X invalid value", Rfa);
                strcat(desc, tmpbuf);
            }
            if (Rfu != 0) {
                sprintf(tmpbuf, ", Rfu=%d invalid value", Rfu);
                strcat(desc, tmpbuf);
            }
            Int_code = f[i] & 0x1F;
            sprintf(tmpbuf, ", Int code=0x%X %s", Int_code, get_programme_type(get_international_table() , Int_code));
            strcat(desc, tmpbuf);
            i++;
        }
        else {
            sprintf(tmpbuf, ", Int code= invalid FIG length");
            strcat(desc, tmpbuf);
        }
        if (CC_flag != 0) {
            if (i < fig0.figlen) {
                Rfa = f[i] >> 6;
                Rfu = (f[i] >> 5) & 0x01;
                if (Rfa != 0) {
                    sprintf(tmpbuf, ", Rfa=0x%X invalid value", Rfa);
                    strcat(desc, tmpbuf);
                }
                if (Rfu != 0) {
                    sprintf(tmpbuf, ", Rfu=%d invalid value", Rfu);
                    strcat(desc, tmpbuf);
                }
                Comp_code = f[i] & 0x1F;
                sprintf(tmpbuf, ", Comp code=0x%X %s", Comp_code, get_programme_type(get_international_table() , Comp_code));
                strcat(desc, tmpbuf);
                i++;
            }
            else {
                sprintf(tmpbuf, ", Comp code= invalid FIG length");
                strcat(desc, tmpbuf);
            }
        }
        printbuf(desc, indent+1, NULL, 0);
    }

    return complete;
}

