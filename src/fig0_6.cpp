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
bool fig0_6(fig0_common_t& fig0, int indent)
{
    uint32_t j;
    uint16_t LSN, key;
    uint8_t i = 1, Number_of_Ids, IdLQ;
    char signal_link[256];
    char desc[256];
    bool Id_list_flag, LA, SH, ILS, Shd;
    bool complete = false;

    uint8_t* f = fig0.f;

    while (i < (fig0.figlen - 1)) {
        // iterate over service linking
        Id_list_flag  =  (f[i] >> 7) & 0x01;
        LA  = (f[i] >> 6) & 0x01;
        SH  = (f[i] >> 5) & 0x01;
        ILS = (f[i] >> 4) & 0x01;
        LSN = ((f[i] & 0x0F) << 8) | f[i+1];
        key = (fig0.oe() << 15) | (fig0.pd() << 14) | (SH << 13) | (ILS << 12) | LSN;
        complete |= fig0_6_is_complete(key);
        strcpy(signal_link, "");
        // check activation / deactivation
        if ((fig0_6_key_la.count(key) > 0) && (fig0_6_key_la[key] != LA)) {
            if (LA == 0) {
                strcat(signal_link, " deactivated");
            }
            else {
                strcat(signal_link, " activated");
            }
        }
        fig0_6_key_la[key] = LA;
        i += 2;
        if (Id_list_flag == 0) {
            if (fig0.cn() == 0) {  // Id_list_flag=0 && fig0.cn()=0: CEI Change Event Indication
                strcat(signal_link, " CEI");
            }
            sprintf(desc, "Id list flag=%d, LA=%d %s, S/H=%d %s, ILS=%d %s, LSN=%d, database key=0x%04x%s",
                    Id_list_flag, LA, (LA)?"active":"inactive", SH, (SH)?"Hard":"Soft", ILS, (ILS)?"international":"national", LSN, key, signal_link);
            printbuf(desc, indent+1, NULL, 0);
        }
        else {  // Id_list_flag == 1
            if (i < fig0.figlen) {
                Number_of_Ids = (f[i] & 0x0F);
                if (fig0.pd() == 0) {
                    IdLQ = (f[i] >> 5) & 0x03;
                    Shd   = (f[i] >> 4) & 0x01;
                    sprintf(desc, "Id list flag=%d, LA=%d %s, S/H=%d %s, ILS=%d %s, LSN=%d, database key=0x%04x, IdLQ=%d, Shd=%d %s, Number of Ids=%d%s",
                            Id_list_flag, LA, (LA)?"active":"inactive", SH, (SH)?"Hard":"Soft", ILS, (ILS)?"international":"national", LSN, key, IdLQ, Shd, (Shd)?"b11-8 in 4-F are different services":"single service", Number_of_Ids, signal_link);
                    printbuf(desc, indent+1, NULL, 0);
                    if (ILS == 0) {
                        // read Id list
                        for(j = 0; ((j < Number_of_Ids) && ((i+2+(j*2)) < fig0.figlen)); j++) {
                            // ETSI EN 300 401 8.1.15:
                            // The IdLQ shall not apply to the first entry in the Id list when fig0.oe() = "0" and 
                            // when the version number of the type 0 field is set to "0" (using the C/N flag, see clause 5.2.2.1)
                            // ... , the first entry in the Id list of each Service linking field shall be 
                            // the SId that applies to the service in the ensemble.
                            if (((j == 0) && (fig0.oe() == 0) && (fig0.cn() == 0)) ||
                                    (IdLQ == 0)) {
                                sprintf(desc, "DAB SId       0x%X", ((f[i+1+(j*2)] << 8) | f[i+2+(j*2)]));
                            }
                            else if (IdLQ == 1) {
                                sprintf(desc, "RDS PI        0x%X", ((f[i+1+(j*2)] << 8) | f[i+2+(j*2)]));
                            }
                            else if (IdLQ == 2) {
                                sprintf(desc, "AM-FM service 0x%X", ((f[i+1+(j*2)] << 8) | f[i+2+(j*2)]));
                            }
                            else {  // IdLQ == 3
                                sprintf(desc, "invalid ILS IdLQ configuration");
                            }
                            printbuf(desc, indent+2, NULL, 0);
                        }
                        // check deadlink
                        if ((Number_of_Ids == 0) && (IdLQ == 1)) {
                            sprintf(desc, "deadlink");
                            printbuf(desc, indent+2, NULL, 0);
                        }
                        i += (Number_of_Ids * 2) + 1;
                    }
                    else {  // fig0.pd() == 0 && ILS == 1
                        // read Id list
                        for(j = 0; ((j < Number_of_Ids) && ((i+3+(j*3)) < fig0.figlen)); j++) {
                            // ETSI EN 300 401 8.1.15:
                            // The IdLQ shall not apply to the first entry in the Id list when fig0.oe() = "0" and 
                            // when the version number of the type 0 field is set to "0" (using the C/N flag, see clause 5.2.2.1)
                            // ... , the first entry in the Id list of each Service linking field shall be 
                            // the SId that applies to the service in the ensemble.
                            if (((j == 0) && (fig0.oe() == 0) && (fig0.cn() == 0)) ||
                                    (IdLQ == 0)) {
                                sprintf(desc, "DAB SId          ecc 0x%02X Id 0x%04X", f[i+1+(j*3)], ((f[i+2+(j*3)] << 8) | f[i+3+(j*3)]));
                            }
                            else if (IdLQ == 1) {
                                sprintf(desc, "RDS PI           ecc 0x%02X Id 0x%04X", f[i+1+(j*3)], ((f[i+2+(j*3)] << 8) | f[i+3+(j*3)]));
                            }
                            else if (IdLQ == 2) {
                                sprintf(desc, "AM-FM service    ecc 0x%02X Id 0x%04X", f[i+1+(j*3)], ((f[i+2+(j*3)] << 8) | f[i+3+(j*3)]));
                            }
                            else {  // IdLQ == 3
                                sprintf(desc, "DRM/AMSS service ecc 0x%02X Id 0x%04X", f[i+1+(j*3)], ((f[i+2+(j*3)] << 8) | f[i+3+(j*3)]));
                            }
                            printbuf(desc, indent+2, NULL, 0);
                        }
                        // check deadlink
                        if ((Number_of_Ids == 0) && (IdLQ == 1)) {
                            sprintf(desc, "deadlink");
                            printbuf(desc, indent+2, NULL, 0);
                        }
                        i += (Number_of_Ids * 3) + 1;
                    }
                }
                else {  // fig0.pd() == 1
                    sprintf(desc, "Id list flag=%d, LA=%d %s, S/H=%d %s, ILS=%d %s, LSN=%d, database key=0x%04x, Number of Ids=%d%s",
                            Id_list_flag, LA, (LA)?"active":"inactive", SH, (SH)?"Hard":"Soft", ILS, (ILS)?"international":"national", LSN, key, Number_of_Ids, signal_link);
                    printbuf(desc, indent+1, NULL, 0);
                    if (Number_of_Ids > 0) {
                        // read Id list
                        for(j = 0; ((j < Number_of_Ids) && ((i+4+(j*4)) < fig0.figlen)); j++) {
                            sprintf(desc, "SId 0x%X",
                                    ((f[i+1+(j*4)] << 24) | (f[i+2+(j*4)] << 16) | (f[i+3+(j*4)] << 8) | f[i+4+(j*4)]));
                            printbuf(desc, indent+2, NULL, 0);
                        }
                    }
                    i += (Number_of_Ids * 4) + 1;
                }
            }
        }
    }

    return complete;
}

