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
#include <set>

/* EN 300 401, 8.1.14.3 Service component label
 * The combination of the SId and the SCIdS provides a service component
 * identifier which is valid globally.
 */
using SId_t = int;
using SCIdS_t = int;
static std::set<std::pair<SId_t, SCIdS_t> > components_seen;

bool fig0_8_is_complete(SId_t SId, SCIdS_t SCIdS)
{
    auto key = std::make_pair(SId, SCIdS);
    bool complete = components_seen.count(key);

    if (complete) {
        components_seen.clear();
    }
    else {
        components_seen.insert(key);
    }

    return complete;
}



// FIG 0/8 Service component global definition
// ETSI EN 300 401 6.3.5
bool fig0_8(fig0_common_t& fig0, int indent)
{
    uint32_t SId;
    uint16_t SCId;
    uint8_t i = 1, Rfa, SCIdS, SubChId, FIDCId;
    char tmpbuf[256];
    char desc[256];
    bool Ext_flag, LS_flag, MSC_FIC_flag;
    uint8_t* f = fig0.f;
    bool complete = false;

    while (i < (fig0.figlen - (2 + (2 * fig0.pd())))) {
        // iterate over service component global definition
        if (fig0.pd() == 0) {
            // Programme services, 16 bit SId
            SId = (f[i] << 8) | f[i+1];
            i += 2;
        }
        else {
            // Data services, 32 bit SId
            SId = ((uint32_t)f[i] << 24) | ((uint32_t)f[i+1] << 16) |
                ((uint32_t)f[i+2] << 8) | (uint32_t)f[i+3];
            i += 4;
        }
        Ext_flag = f[i] >> 7;
        Rfa = (f[i] >> 4) & 0x7;
        SCIdS = f[i] & 0x0F;
        complete |= fig0_8_is_complete(SId, SCIdS);
        sprintf(desc, "SId=0x%X, Ext flag=%d 8-bit Rfa %s", SId, Ext_flag, (Ext_flag)?"present":"absent");
        if (Rfa != 0) {
            sprintf(tmpbuf, ", Rfa=%d invalid value", Rfa);
            strcat(desc, tmpbuf);
        }
        sprintf(tmpbuf, ", SCIdS=0x%X", SCIdS);
        strcat(desc, tmpbuf);
        i++;
        if (i < fig0.figlen) {
            LS_flag = f[i] >> 7;
            sprintf(tmpbuf, ", L/S flag=%d %s", LS_flag, (LS_flag)?"Long form":"Short form");
            strcat(desc, tmpbuf);
            if (LS_flag == 0) {
                // Short form
                if (i < (fig0.figlen - Ext_flag)) {
                    MSC_FIC_flag = (f[i] >> 6) & 0x01;
                    if (MSC_FIC_flag == 0) {
                        // MSC in stream mode and SubChId identifies the sub-channel
                        SubChId = f[i] & 0x3F;
                        sprintf(tmpbuf, ", MSC/FIC flag=%d MSC, SubChId=0x%X", MSC_FIC_flag, SubChId);
                        strcat(desc, tmpbuf);
                    }
                    else {
                        // FIC and FIDCId identifies the component
                        FIDCId = f[i] & 0x3F;
                        sprintf(tmpbuf, ", MSC/FIC flag=%d FIC, FIDCId=0x%X", MSC_FIC_flag, FIDCId);
                        strcat(desc, tmpbuf);
                    }
                    if (Ext_flag == 1) {
                        // Rfa field present
                        Rfa = f[i+1];
                        if (Rfa != 0) {
                            sprintf(tmpbuf, ", Rfa=0x%X invalid value", Rfa);
                            strcat(desc, tmpbuf);
                        }
                    }
                }
                i += (1 + Ext_flag);
            }
            else {
                // Long form
                if (i < (fig0.figlen - 1)) {
                    Rfa = (f[i] >> 4) & 0x07;
                    SCId = (((uint16_t)f[i] & 0x0F) << 8) | (uint16_t)f[i+1];
                    if (Rfa != 0) {
                        sprintf(tmpbuf, ", Rfa=%d invalid value", Rfa);
                        strcat(desc, tmpbuf);
                    }
                    sprintf(tmpbuf, ", SCId=0x%X", SCId);
                    strcat(desc, tmpbuf);
                }
                i += 2;
            }
        }
        printbuf(desc, indent+1, NULL, 0);
    }

    return complete;
}

