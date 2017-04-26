/*
    Copyright (C) 2014 CSP Innovazione nelle ICT s.c.a r.l. (http://www.csp.it/)
    Copyright (C) 2017 Matthias P. Braendli (http://www.opendigitalradio.org)
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

static std::unordered_set<int> regions_seen;

bool fig0_21_is_complete(int region_id)
{
    bool complete = regions_seen.count(region_id);

    if (complete) {
        regions_seen.clear();
    }
    else {
        regions_seen.insert(region_id);
    }

    return complete;
}


// FIG 0/21 Frequency Information
// ETSI EN 300 401 8.1.8
fig_result_t fig0_21(fig0_common_t& fig0, const display_settings_t &disp)
{
    float freq;
    uint32_t ifreq;
    uint64_t key;
    uint16_t RegionId, Id_field;
    uint8_t i = 1, j, k, Length_FI_list, RandM, Length_Freq_list, Control_field;
    uint8_t Control_field_trans_mode, Id_field2;
    fig_result_t r;
    bool Continuity_flag;
    uint8_t* f = fig0.f;

    while (i < (fig0.figlen - 1)) {
        // iterate over frequency information
        // decode RegionId, Length of FI list
        RegionId  =  (f[i] << 3) | (f[i+1] >> 5);
        r.complete |= fig0_21_is_complete(RegionId);
        Length_FI_list  = (f[i+1] & 0x1F);
        r.msgs.push_back(strprintf("RegionId=0x%03x", RegionId));
        r.msgs.push_back(strprintf("Len=%d", Length_FI_list));
        i += 2;
        if ((i + Length_FI_list) <= fig0.figlen) {
            j = i;
            while ((j + 2) < (i + Length_FI_list)) {
                // iterate over FI list x
                // decode Id field, R&M, Continuity flag, Length of Freq list
                Id_field = (f[j] << 8) | f[j+1];
                RandM = f[j+2] >> 4;
                Continuity_flag = (f[j+2] >> 3) & 0x01;
                Length_Freq_list = f[j+2] & 0x07;
                std::string idfield;
                switch (RandM) {
                    case 0x0:
                    case 0x1:
                        idfield += "EId";
                        break;
                    case 0x6:
                        idfield += "DRM Service Id";
                        break;
                    case 0x8:
                        idfield += "RDS PI";
                        break;
                    case 0x9:
                    case 0xa:
                    case 0xc:
                        idfield += "Dummy";
                        break;
                    case 0xe:
                        idfield += "AMSS Service Id";
                        break;
                    default:
                        idfield += "invalid";
                        r.errors.emplace_back("R&M invalid");
                        break;
                }
                r.msgs.emplace_back(1, strprintf("ID field=0x%X", Id_field) + idfield);

                std::string rm_str;
                switch (RandM) {
                    case 0x0:
                        rm_str += " DAB ensemble, no local windows";
                        break;
                    case 0x6:
                        rm_str += " DRM";
                        break;
                    case 0x8:
                        rm_str += " FM with RDS";
                        break;
                    case 0x9:
                        rm_str += " FM without RDS";
                        break;
                    case 0xa:
                        rm_str += " AM (MW in 9 kHz steps & LW)";
                        break;
                    case 0xc:
                        rm_str += " AM (MW in 5 kHz steps & SW)";
                        break;
                    case 0xe:
                        rm_str += " AMSS";
                        break;
                    default:
                        rm_str += " Rfu";
                        r.errors.emplace_back("R&M is Rfu");
                        break;
                }
                r.msgs.emplace_back(1, strprintf("R&M=0x%1x", RandM) + rm_str);
                std::string continuity_str;
                if ((fig0.oe() == 0) || ((fig0.oe() == 1) && (RandM != 0x6) &&
                            ((RandM < 0x8) || (RandM > 0xa)) && (RandM != 0xc) && (RandM != 0xe))) {
                    if (Continuity_flag == 0) {
                        switch (RandM) {
                            case 0x0:
                            case 0x1:
                                continuity_str += "=continuous output not expected";
                                break;
                            case 0x6:
                                continuity_str += "=no compensating time delay on DRM audio signal";
                                break;
                            case 0x8:
                            case 0x9:
                                continuity_str += "=no compensating time delay on FM audio signal";
                                break;
                            case 0xa:
                            case 0xc:
                            case 0xe:
                                continuity_str += "=no compensating time delay on AM audio signal";
                                break;
                            default:
                                continuity_str += "=Rfu";
                                break;
                        }
                    }
                    else {  // Continuity_flag == 1
                        switch (RandM) {
                            case 0x0:
                            case 0x1:
                                continuity_str += "=continuous output possible";
                                break;
                            case 0x6:
                                continuity_str += "=compensating time delay on DRM audio signal";
                                break;
                            case 0x8:
                            case 0x9:
                                continuity_str += "=compensating time delay on FM audio signal";
                                break;
                            case 0xa:
                            case 0xc:
                            case 0xe:
                                continuity_str += "=compensating time delay on AM audio signal";
                                break;
                            default:
                                continuity_str += "=Rfu";
                                r.errors.emplace_back("continuity is Rfu");
                                break;
                        }
                    }
                }
                else {  // fig0.oe() == 1
                    continuity_str = "=reserved for future addition";
                    r.errors.emplace_back("Rfu");
                }

                r.msgs.emplace_back(1, strprintf("Continuity flag=%d ", Continuity_flag) + continuity_str);

                key = ((uint64_t)fig0.oe() << 32) | ((uint64_t)fig0.pd() << 31) | \
                      ((uint64_t)RegionId << 20) | ((uint64_t)Id_field << 4) | \
                      (uint64_t)RandM;
                r.msgs.emplace_back(1, strprintf("database key=0x%09" PRId64, key));
                // CEI Change Event Indication
                if (Length_Freq_list == 0) {
                    r.msgs.emplace_back(1, "CEI");
                }
                j += 3; // add header

                k = j;
                switch (RandM) {
                    case 0x0:
                    case 0x1:
                        while((k + 2) < (j + Length_Freq_list)) {
                            // iteration over Freq list
                            ifreq = (((uint32_t)(f[k] & 0x07) << 16) | ((uint32_t)f[k+1] << 8) | (uint32_t)f[k+2]) * 16;
                            if (ifreq != 0) {
                                Control_field = (f[k] >> 3);
                                Control_field_trans_mode = (Control_field >> 1) & 0x07;
                                if ((Control_field & 0x10) == 0) {
                                    r.msgs.emplace_back(2, strprintf("%d KHz", ifreq));
                                    if ((Control_field & 0x01) == 0) {
                                        r.msgs.emplace_back(2, "geographically adjacent area");
                                    }
                                    else {  // (Control_field & 0x01) == 1
                                        r.msgs.emplace_back(2, "no geographically adjacent area");
                                    }
                                    if (Control_field_trans_mode == 0) {
                                        r.msgs.emplace_back(2, "no transmission mode signalled");
                                    }
                                    else if (Control_field_trans_mode <= 4) {
                                        r.msgs.emplace_back(2, strprintf("transmission mode %d", Control_field_trans_mode));
                                    }
                                    else {  // Control_field_trans_mode > 4
                                        r.msgs.emplace_back(2, strprintf("invalid transmission mode 0x%x", Control_field_trans_mode));
                                    }
                                }
                                else {  // (Control_field & 0x10) == 0x10
                                    r.msgs.emplace_back(2, strprintf("%d KHz, invalid Control field b23 0x%x", ifreq, Control_field));
                                }
                            }
                            else {
                                r.errors.emplace_back("Frequency not to be used (0)");
                            }
                            k += 3;
                        }
                        break;
                    case 0x8:
                    case 0x9:
                    case 0xa:
                        while(k < (j + Length_Freq_list)) {
                            // iteration over Freq list
                            if (f[k] != 0) {    // freq != 0
                                if (RandM == 0xa) {
                                    if (f[k] < 16) {
                                        ifreq = (144 + ((uint32_t)f[k] * 9));
                                    }
                                    else {  // f[k] >= 16
                                        ifreq = (387 + ((uint32_t)f[k] * 9));
                                    }
                                    r.msgs.emplace_back(2, strprintf("%d KHz", ifreq));
                                }
                                else {  // RandM == 8 or 9
                                    freq = (87.5 + ((float)f[k] * 0.1));
                                    r.msgs.emplace_back(2, strprintf("%.1f MHz", freq));
                                }
                            }
                            else {
                                r.errors.emplace_back("Frequency not to be used (0)");
                            }
                            k++;
                        }
                        break;
                    case 0xc:
                        while((k + 1) < (j + Length_Freq_list)) {
                            // iteration over Freq list
                            ifreq = (((uint32_t)f[k] << 8) | (uint32_t)f[k+1]) * 5;
                            if (ifreq != 0) {
                                r.msgs.emplace_back(2, strprintf("%d KHz", ifreq));
                            }
                            else {
                                r.errors.emplace_back("Frequency not to be used (0)");
                            }
                            k += 2;
                        }
                        break;
                    case 0x6:
                    case 0xe:
                        while((k + 2) < (j + Length_Freq_list)) {
                            // iteration over Freq list
                            Id_field2 = f[k];
                            ifreq = ((((uint32_t)f[k+1] & 0x7f) << 8) | (uint32_t)f[k+2]);
                            if (ifreq != 0) {
                                r.msgs.emplace_back(2, strprintf("%d KHz", ifreq));
                            }
                            else {
                                r.errors.emplace_back("Frequency not to be used (0)");
                            }
                            if (RandM == 0x6) {
                                r.msgs.emplace_back(2, strprintf("DRM Service Id 0x%X", Id_field2));
                            }
                            else if (RandM == 0xe) {
                                r.msgs.emplace_back(2, strprintf("AMSS Service Id 0x%X", Id_field2));
                            }
                            if ((f[k+1] & 0x80) == 0x80) {
                                r.msgs.emplace_back(2, strprintf("invalid Rfu b15 set to 1 instead of 0"));
                            }
                            k += 3;
                        }
                        break;
                    default:
                        break;
                }
                j += Length_Freq_list;
            }
            i += Length_FI_list;
        }
        else {
            r.errors.push_back(strprintf("FIG0/21 FI Len error: expect %d + %d <= %d\n",
                    i , Length_FI_list, fig0.figlen));
        }
    }

    return r;
}

