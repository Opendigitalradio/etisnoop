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
#include <unordered_set>

static std::unordered_set<int> components_ids_seen;

bool fig0_3_is_complete(int components_id)
{
    bool complete = components_ids_seen.count(components_id);

    if (complete) {
        components_ids_seen.clear();
    }
    else {
        components_ids_seen.insert(components_id);
    }

    return complete;
}


// FIG 0/3 Service component in packet mode with or without Conditional Access
// ETSI EN 300 401 6.3.2
bool fig0_3(fig0_common_t& fig0, int indent)
{
    uint16_t SCId, Packet_address, CAOrg;
    uint8_t i = 1, Rfa, DSCTy, SubChId, CAMode, SharedFlag;
    char tmpbuf[256];
    char desc[256];
    bool CAOrg_flag, DG_flag, Rfu;
    bool complete = false;

    uint8_t* f = fig0.f;

    while (i < (fig0.figlen - 4)) {
        // iterate over service component in packet mode
        SCId = ((uint16_t)f[i] << 4) | ((uint16_t)(f[i+1] >> 4) & 0x0F);
        complete |= fig0_3_is_complete(SCId);
        Rfa = (f[i+1] >> 1) & 0x07;
        CAOrg_flag = f[i+1] & 0x01;
        DG_flag = (f[i+2] >> 7) & 0x01;
        Rfu = (f[i+2] >> 6) & 0x01;
        DSCTy = f[i+2] & 0x3F;
        SubChId = (f[i+3] >> 2);
        Packet_address = ((uint16_t)(f[i+3] & 0x03) << 8) | ((uint16_t)f[i+4]);
        sprintf(desc,
                "SCId=0x%X, CAOrg flag=%d CAOrg field %s, DG flag=%d"
                " data groups are %sused to transport the service component,"
                " DSCTy=%d %s, SubChId=0x%X, Packet address=0x%X",
                SCId, CAOrg_flag, CAOrg_flag?"present":"absent", DG_flag,
                DG_flag ? "not ": "",
                DSCTy, get_dscty_type(DSCTy), SubChId,
                Packet_address);
        if (Rfa != 0) {
            sprintf(tmpbuf, ", Rfa=%d invalid value", Rfa);
            strcat(desc, tmpbuf);
        }
        if (Rfu != 0) {
            sprintf(tmpbuf, ", Rfu=%d invalid value", Rfu);
            strcat(desc, tmpbuf);
        }
        i += 5;
        if (CAOrg_flag) {
            if (i < (fig0.figlen - 1)) {
                CAOrg = ((uint16_t)f[i] << 8) | ((uint16_t)f[i+1]);
                CAMode = (f[i] >> 5);
                SharedFlag = f[i+1];
                sprintf(tmpbuf, ", CAOrg=0x%X CAMode=%d \"%s\" SharedFlag=0x%X%s",
                        CAOrg, CAMode, get_ca_mode(CAMode), SharedFlag, (SharedFlag == 0) ? " invalid" : "");
                strcat(desc, tmpbuf);
            }
            else {
                sprintf(tmpbuf, ", invalid figlen");
                strcat(desc, tmpbuf);
            }
            i += 2;
        }
        printbuf(desc, indent+1, NULL, 0);
    }

    return complete;
}

