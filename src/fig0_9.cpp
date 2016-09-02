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

// fig 0/9 global variables
uint8_t Ensemble_ECC=0;
int8_t Ensemble_LTO=0;
bool LTO_uniq;


// FIG 0/9 Country, LTO and International table
// ETSI EN 300 401 8.1.3.2
bool fig0_9(fig0_common_t& fig0, int indent)
{
    uint32_t SId;
    uint8_t i = 1, j, key, Number_of_services, ECC;
    int8_t LTO;
    char tmpbuf[256];
    char desc[256];
    bool Ext_flag;
    uint8_t* f = fig0.f;

    if (i < (fig0.figlen - 2)) {
        // get Ensemble LTO, ECC and International Table Id
        key = ((uint8_t)fig0.oe() << 1) | (uint8_t)fig0.pd();
        Ext_flag = f[i] >> 7;
        LTO_uniq = (f[i]>> 6) & 0x01;
        Ensemble_LTO = f[i] & 0x3F;
        if (Ensemble_LTO & 0x20) {
            // negative Ensemble LTO
            Ensemble_LTO |= 0xC0;
        }
        sprintf(desc, "Ext flag=%d extended field %s, LTO uniq=%d %s, Ensemble LTO=0x%X %s%d:%02d",
                Ext_flag, Ext_flag?"present":"absent", LTO_uniq,
                LTO_uniq?"several time zones":"one time zone (time specified by Ensemble LTO)",
                (Ensemble_LTO & 0x3F), (Ensemble_LTO >= 0)?"":"-" , abs(Ensemble_LTO) >> 1, (Ensemble_LTO & 0x01) * 30);
        if (abs(Ensemble_LTO) > 24) {
            sprintf(tmpbuf, " out of range -12 hours to +12 hours");
            strcat(desc, tmpbuf);
        }
        Ensemble_ECC = f[i+1];
        uint8_t International_Table_Id = f[i+2];
        set_international_table(International_Table_Id);
        sprintf(tmpbuf, ", Ensemble ECC=0x%X, International Table Id=0x%X, database key=0x%x",
                Ensemble_ECC, International_Table_Id, key);
        strcat(desc, tmpbuf);
        printbuf(desc, indent+1, NULL, 0);
        i += 3;
        if (Ext_flag == 1) {
            // extended field present
            while (i < fig0.figlen) {
                // iterate over extended sub-field
                Number_of_services = f[i] >> 6;
                LTO = f[i] & 0x3F;
                if (LTO & 0x20) {
                    // negative LTO
                    LTO |= 0xC0;
                }
                sprintf(desc, "Number of services=%d, LTO=0x%X %s%d:%02d",
                        Number_of_services, (LTO & 0x3F), (LTO >= 0)?"":"-" , abs(LTO) >> 1,  (LTO & 0x01) * 30);
                if (abs(LTO) > 24) {
                    sprintf(tmpbuf, " out of range -12 hours to +12 hours");
                    strcat(desc, tmpbuf);
                }
                // CEI Change Event Indication
                if ((Number_of_services == 0) && (LTO == 0) /* && (Ext_flag == 1) */) {
                    sprintf(tmpbuf, ", CEI");
                    strcat(desc, tmpbuf);
                }
                i++;
                if (fig0.pd() == 0) {
                    // Programme services, 16 bit SId
                    if (i < fig0.figlen) {
                        ECC = f[i];
                        sprintf(tmpbuf, ", ECC=0x%X", ECC);
                        strcat(desc, tmpbuf);
                        printbuf(desc, indent+2, NULL, 0);
                        i++;
                        for(j = i; ((j < (i + (Number_of_services * 2))) && (j < fig0.figlen)); j += 2) {
                            // iterate over SId
                            SId = ((uint32_t)f[j] << 8) | (uint32_t)f[j+1];
                            sprintf(desc, "SId 0x%X", SId);
                            printbuf(desc, indent+3, NULL, 0);
                        }
                        i += (Number_of_services * 2);
                    }
                }
                else {
                    // Data services, 32 bit SId
                    printbuf(desc, indent+2, NULL, 0);
                    for(j = i; ((j < (i + (Number_of_services * 4))) && (j < fig0.figlen)); j += 4) {
                        // iterate over SId
                        SId = ((uint32_t)f[j] << 24) | ((uint32_t)f[j+1] << 16) |
                            ((uint32_t)f[j+2] << 8) | (uint32_t)f[j+3];
                        sprintf(desc, "SId 0x%X", SId);
                        printbuf(desc, indent+3, NULL, 0);
                    }
                    i += (Number_of_services * 4);
                }
            }
        }
    }

    return true;
}

