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
bool fig0_5(fig0_common_t& fig0, int indent)
{
    uint16_t SCId;
    uint8_t i = 1, SubChId, FIDCId, Language, Rfa;
    char tmpbuf[256];
    char desc[256];
    bool LS_flag, MSC_FIC_flag;
    bool complete = false;

    uint8_t* f = fig0.f;

    while (i < (fig0.figlen - 1)) {
        // iterate over service component language
        LS_flag = f[i] >> 7;
        if (LS_flag == 0) {
            // Short form (L/S = 0)
            MSC_FIC_flag = (f[i] >> 6) & 0x01;
            Language = f[i+1];
            if (MSC_FIC_flag == 0) {
                // 0: MSC in Stream mode and SubChId identifies the sub-channel
                SubChId = f[i] & 0x3F;
                sprintf(desc, "L/S flag=%d short form, MSC/FIC flag=%d MSC, SubChId=0x%X, Language=0x%X %s",
                        LS_flag, MSC_FIC_flag, SubChId, Language,
                        get_language_name(Language));
            }
            else {
                // 1: FIC and FIDCId identifies the component
                FIDCId = f[i] & 0x3F;
                sprintf(desc, "L/S flag=%d short form, MSC/FIC flag=%d FIC, FIDCId=0x%X, Language=0x%X %s",
                        LS_flag, MSC_FIC_flag, FIDCId, Language,
                        get_language_name(Language));
            }

            int key = (MSC_FIC_flag << 7) | (f[i] % 0x3F);
            complete |= fig0_5_is_complete(key);
            printbuf(desc, indent+1, NULL, 0);
            i += 2;
        }
        else {
            // Long form (L/S = 1)
            if (i < (fig0.figlen - 2)) {
                Rfa = (f[i] >> 4) & 0x07;
                SCId = (((uint16_t)f[i] & 0x0F) << 8) | (uint16_t)f[i+1];
                int key = (LS_flag << 15) | SCId;
                complete |= fig0_5_is_complete(key);
                Language = f[i+2];
                sprintf(desc, "L/S flag=%d long form", LS_flag);
                if (Rfa != 0) {
                    sprintf(tmpbuf, ", Rfa=%d invalid value", Rfa);
                    strcat(desc, tmpbuf);
                }
                sprintf(tmpbuf, ", SCId=0x%X, Language=0x%X %s",
                        SCId, Language,
                        get_language_name(Language));
                strcat(desc, tmpbuf);
                printbuf(desc, indent+1, NULL, 0);
            }
            i += 3;
        }
    }

    return complete;
}

