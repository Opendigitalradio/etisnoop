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

static std::unordered_set<int> links_seen;

bool fig0_6_is_complete(int link_key)
{
    bool complete = links_seen.count(link_key);

    if (complete) {
        links_seen.clear();
    }
    else {
        links_seen.insert(link_key);
    }

    return complete;
}

// map between fig 0/6 database key and LA to detect activation and deactivation of links
static std::map<uint16_t, bool> fig0_6_key_la;

void fig0_6_cleardb()
{
    fig0_6_key_la.clear();
}

// FIG 0/6 Service linking information
// ETSI EN 300 401 8.1.15
fig_result_t fig0_6(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint32_t j;
    uint16_t LSN, key;
    uint8_t i = 1, Number_of_Ids, IdLQ;
    fig_result_t r;
    bool Id_list_flag, LA, SH, ILS, Shd;

    uint8_t* f = fig0.f;

    while (i < (fig0.figlen - 1)) {
        // iterate over service linking
        Id_list_flag  =  (f[i] >> 7) & 0x01;
        LA  = (f[i] >> 6) & 0x01;
        SH  = (f[i] >> 5) & 0x01;
        ILS = (f[i] >> 4) & 0x01;
        LSN = ((f[i] & 0x0F) << 8) | f[i+1];
        key = (fig0.oe() << 15) | (fig0.pd() << 14) | (SH << 13) | (ILS << 12) | LSN;
        r.complete |= fig0_6_is_complete(key);

        r.msgs.emplace_back(0, "-");
        r.msgs.emplace_back(1, strprintf("Id list flag=%d", Id_list_flag));
        r.msgs.emplace_back(1, strprintf("LA=%d %s", LA, LA ? "active" : "inactive"));
        r.msgs.emplace_back(1, strprintf("S/H=%d %s", SH, SH ? "Hard" : "Soft"));
        r.msgs.emplace_back(1, strprintf("ILS=%d %s", ILS, ILS ? "international" : "national"));
        r.msgs.emplace_back(1, strprintf("LSN=%d", LSN));
        r.msgs.emplace_back(1, strprintf("database key=0x%04x", key));

        // check activation / deactivation
        if ((fig0_6_key_la.count(key) > 0) && (fig0_6_key_la[key] != LA)) {
            if (LA == 0) {
                r.msgs.emplace_back(1, "status=deactivated");
            }
            else {
                r.msgs.emplace_back(1, "status=activated");
            }
        }
        fig0_6_key_la[key] = LA;
        i += 2;
        if (Id_list_flag == 0) {
            if (fig0.cn() == 0) {  // Id_list_flag=0 && fig0.cn()=0: CEI Change Event Indication
                r.msgs.emplace_back(1, "CEI=true");
            }
        }
        else {  // Id_list_flag == 1
            if (i < fig0.figlen) {
                Number_of_Ids = (f[i] & 0x0F);
                if (fig0.pd() == 0) {
                    IdLQ = (f[i] >> 5) & 0x03;
                    Shd   = (f[i] >> 4) & 0x01;
                    r.msgs.emplace_back(1, strprintf("IdLQ=%d", IdLQ));
                    r.msgs.emplace_back(1, strprintf("Shd=%d %s", Shd, (Shd)?"b11-8 in 4-F are different services":"single service"));

                    if (ILS == 0) {
                        // read Id list
                        r.msgs.emplace_back(1, "Id List:");
                        for(j = 0; ((j < Number_of_Ids) && ((i+2+(j*2)) < fig0.figlen)); j++) {
                            r.msgs.emplace_back(2, "-");
                            // ETSI EN 300 401 8.1.15. Some changes were introducted in spec V2
                            if (((j == 0) && (fig0.oe() == 0) && (fig0.cn() == 0)) ||
                                    (IdLQ == 0)) {
                                r.msgs.emplace_back(3, strprintf("DAB SId=0x%X",
                                            ((f[i+1+(j*2)] << 8) | f[i+2+(j*2)])));
                            }
                            else if (IdLQ == 1) {
                                r.msgs.emplace_back(3, strprintf("RDS PI=0x%X",
                                            ((f[i+1+(j*2)] << 8) | f[i+2+(j*2)])));
                            }
                            else if (IdLQ == 2) {
                                r.msgs.emplace_back(3, strprintf("(AM-FM legacy)=0x%X",
                                            ((f[i+1+(j*2)] << 8) | f[i+2+(j*2)])));
                            }
                            else {  // IdLQ == 3
                                r.msgs.emplace_back(3, strprintf("DRM-AMSS service=0x%X",
                                            ((f[i+1+(j*2)] << 8) | f[i+2+(j*2)])));
                            }
                        }

                        // check deadlink
                        if ((Number_of_Ids == 0) && (IdLQ == 1)) {
                            r.errors.push_back("deadlink");
                        }
                        i += (Number_of_Ids * 2) + 1;
                    }
                    else {  // fig0.pd() == 0 && ILS == 1
                        r.msgs.emplace_back(1, "Id List:");
                        // read Id list
                        for(j = 0; ((j < Number_of_Ids) && ((i+3+(j*3)) < fig0.figlen)); j++) {
                            r.msgs.emplace_back(2, "-");
                            if (((j == 0) && (fig0.oe() == 0) && (fig0.cn() == 0)) ||
                                    (IdLQ == 0)) {
                                r.msgs.emplace_back(3, strprintf("DAB SId=ecc 0x%02X Id 0x%04X",
                                            f[i+1+(j*3)], ((f[i+2+(j*3)] << 8) | f[i+3+(j*3)])));
                            }
                            else if (IdLQ == 1) {
                                r.msgs.emplace_back(3, strprintf("RDS PI=ecc 0x%02X Id 0x%04X",
                                            f[i+1+(j*3)], ((f[i+2+(j*3)] << 8) | f[i+3+(j*3)])));
                            }
                            else if (IdLQ == 2) {
                                r.msgs.emplace_back(3, strprintf("(AM-FM legacy)=ecc 0x%02X Id 0x%04X",
                                            f[i+1+(j*3)], ((f[i+2+(j*3)] << 8) | f[i+3+(j*3)])));
                            }
                            else {  // IdLQ == 3
                                r.msgs.emplace_back(3, strprintf("DRM/AMSS service=ecc 0x%02X Id 0x%04X",
                                            f[i+1+(j*3)], ((f[i+2+(j*3)] << 8) | f[i+3+(j*3)])));
                            }
                        }
                        // check deadlink
                        if ((Number_of_Ids == 0) && (IdLQ == 1)) {
                            r.errors.push_back("deadlink");
                        }
                        i += (Number_of_Ids * 3) + 1;
                    }
                }
                else {  // fig0.pd() == 1
                    r.msgs.emplace_back(1, "Id List:");
                    if (Number_of_Ids > 0) {
                        // read Id list
                        for(j = 0; ((j < Number_of_Ids) && ((i+4+(j*4)) < fig0.figlen)); j++) {
                            r.msgs.emplace_back(2, strprintf("- 0x%X",
                                    ((f[i+1+(j*4)] << 24) | (f[i+2+(j*4)] << 16) | (f[i+3+(j*4)] << 8) | f[i+4+(j*4)])));
                        }
                    }
                    i += (Number_of_Ids * 4) + 1;
                }
            }
        }
    }

    return r;
}

