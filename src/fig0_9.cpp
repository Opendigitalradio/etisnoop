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


// FIG 0/9 Country, LTO and International table
// ETSI EN 300 401 8.1.3.2
fig_result_t fig0_9(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint32_t SId;
    uint8_t Ensemble_ECC=0;
    uint8_t i = 1, j, key, Number_of_services, ECC;
    int8_t LTO;
    bool LTO_uniq;
    fig_result_t r;
    bool Ext_flag;
    uint8_t* f = fig0.f;

    if (i < (fig0.figlen - 2)) {
        // get Ensemble LTO, ECC and International Table Id
        key = ((uint8_t)fig0.oe() << 1) | (uint8_t)fig0.pd();
        Ext_flag = f[i] >> 7;
        LTO_uniq = (f[i]>> 6) & 0x01;
        int8_t Ensemble_LTO = f[i] & 0x3F;
        if (Ensemble_LTO & 0x20) {
            // negative Ensemble LTO
            Ensemble_LTO |= 0xC0;
        }
        r.msgs.emplace_back("-");
        r.msgs.emplace_back(1, strprintf("Ext flag=%d extended field %s",
                    Ext_flag, Ext_flag?"present":"absent"));
        r.msgs.emplace_back(1, strprintf("LTO uniq=%d %s",
                    LTO_uniq,
                    LTO_uniq?"several time zones":"one time zone (time specified by Ensemble LTO)"));
        r.msgs.emplace_back(1, strprintf("Ensemble LTO=0x%X %s%d:%02d",
                    (Ensemble_LTO & 0x3F), (Ensemble_LTO >= 0)?"":"-" , abs(Ensemble_LTO) >> 1, (Ensemble_LTO & 0x01) * 30));

        if (abs(Ensemble_LTO) > 24) {
            r.errors.push_back("LTO out of range -12 hours to +12 hours");
        }

        Ensemble_ECC = f[i+1];
        uint8_t International_Table_Id = f[i+2];
        set_international_table(International_Table_Id);
        r.msgs.emplace_back(1, strprintf("Ensemble ECC=0x%X", Ensemble_ECC));
        r.msgs.emplace_back(1, strprintf("International Table Id=0x%X", International_Table_Id));
        r.msgs.emplace_back(1, strprintf("database key=0x%x", key));

        i += 3;
        if (Ext_flag == 1) {
            // extended field present
            r.msgs.emplace_back(1, "Subfields:");
            while (i < fig0.figlen) {
                // iterate over extended sub-field
                Number_of_services = f[i] >> 6;
                LTO = f[i] & 0x3F;
                if (LTO & 0x20) {
                    // negative LTO
                    LTO |= 0xC0;
                }
                r.msgs.emplace_back(2, "-");
                r.msgs.emplace_back(3, strprintf("Number of services=%d", Number_of_services));
                r.msgs.emplace_back(3, strprintf("LTO=0x%X %s%d:%02d",
                        (LTO & 0x3F), (LTO >= 0)?"":"-" , abs(LTO) >> 1,  (LTO & 0x01) * 30));
                if (abs(LTO) > 24) {
                    r.errors.push_back("LTO in extended field out of range -12 hours to +12 hours");
                }

                // CEI Change Event Indication
                if ((Number_of_services == 0) && (LTO == 0) /* && (Ext_flag == 1) */) {
                    r.msgs.emplace_back(3, "CEI=true");
                }
                i++;

                if (fig0.pd() == 0) {
                    // Programme services, 16 bit SId
                    if (i < fig0.figlen) {
                        ECC = f[i];
                        r.msgs.emplace_back(3, strprintf("ECC=0x%X", ECC));
                        i++;
                        for(j = i; ((j < (i + (Number_of_services * 2))) && (j < fig0.figlen)); j += 2) {
                            // iterate over SId
                            SId = ((uint32_t)f[j] << 8) | (uint32_t)f[j+1];
                            r.msgs.emplace_back(3, strprintf("SId=0x%X", SId));
                        }
                        i += (Number_of_services * 2);
                    }
                }
                else {
                    // Data services, 32 bit SId
                    for(j = i; ((j < (i + (Number_of_services * 4))) && (j < fig0.figlen)); j += 4) {
                        // iterate over SId
                        SId = ((uint32_t)f[j] << 24) | ((uint32_t)f[j+1] << 16) |
                            ((uint32_t)f[j+2] << 8) | (uint32_t)f[j+3];
                        r.msgs.emplace_back(3, strprintf("SId=0x%X", SId));
                    }
                    i += (Number_of_services * 4);
                }
            }
        }
    }

    r.complete = true;
    return r;
}

