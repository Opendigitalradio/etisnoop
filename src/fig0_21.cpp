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
    uint8_t* f = fig0.f;
    fig_result_t r;

    int i = 1;
    while (i < fig0.figlen) {
        const uint16_t RegionId = (f[i] << 3) | (f[i+1] >> 5);
        r.complete |= fig0_21_is_complete(RegionId);
        const uint8_t Length_FI_list = f[i+1] & 0x1F; // in bytes
        r.msgs.emplace_back("-");
        r.msgs.emplace_back(1, strprintf("RegionId=0x%03x", RegionId));
        r.msgs.emplace_back(1, strprintf("Len=%d Bytes", Length_FI_list));
        i += 2;
        const int FI_start_ix = i;

        r.msgs.emplace_back(1, "FIs:");
        for (size_t FI_ix = 0; i < FI_start_ix + Length_FI_list; FI_ix++) {
            if (i + 3 > fig0.figlen) {
                r.errors.push_back("FIG0/21 too small!");
                break;
            }

            const uint16_t Id_field = (f[i] << 8) | f[i+1];
            const uint8_t RandM = f[i+2] >> 4;
            const bool Continuity_flag = (f[i+2] >> 3) & 0x01;
            const uint8_t Length_Freq_list = f[i+2] & 0x07; // in bytes
            r.msgs.emplace_back(2, "-");
            r.msgs.emplace_back(3, strprintf("Length Freq list=%d", Length_Freq_list));
            i += 3;

            std::string idfield;
            switch (RandM) {
                case 0x0:
                case 0x1: idfield = "EId"; break;
                case 0x6: idfield = "DRM Service Id"; break;
                case 0x8: idfield = "RDS PI"; break;
                case 0x9:
                case 0xa:
                case 0xc: idfield = "Dummy"; break;
                case 0xe: idfield = "AMSS Service Id"; break;
                default:
                          idfield = "invalid";
                          r.errors.emplace_back("R&M invalid");
                          break;
            }
            r.msgs.emplace_back(3,
                    strprintf("ID field=0x%X ", Id_field) + idfield);

            std::string rm_str;
            switch (RandM) {
                case 0x0: rm_str = "DAB ensemble, no local windows"; break;
                case 0x6: rm_str = "DRM"; break;
                case 0x8: rm_str = "FM with RDS"; break;
                case 0x9: rm_str = "FM without RDS"; break;
                case 0xa: rm_str = "AM (MW in 9 kHz steps & LW)"; break;
                case 0xc: rm_str = "AM (MW in 5 kHz steps & SW)"; break;
                case 0xe: rm_str = "AMSS"; break;
                default:
                          rm_str = "Rfu";
                          r.errors.emplace_back("R&M is Rfu");
                          break;
            }
            r.msgs.emplace_back(3,
                    strprintf("R&M=0x%1x ", RandM) + rm_str);

            std::string continuity_str;
            if ((fig0.oe() == 0) || ((fig0.oe() == 1) && (RandM != 0x6) &&
                        ((RandM < 0x8) || (RandM > 0xa)) && (RandM != 0xc) && (RandM != 0xe))) {
                if (Continuity_flag == 0) {
                    switch (RandM) {
                        case 0x0:
                        case 0x1:
                            continuity_str = ", continuous output not expected";
                            break;
                        case 0x6:
                            continuity_str = ", no compensating time delay on DRM audio signal";
                            break;
                        case 0x8:
                        case 0x9:
                            continuity_str = ", no compensating time delay on FM audio signal";
                            break;
                        case 0xa:
                        case 0xc:
                        case 0xe:
                            continuity_str = ", no compensating time delay on AM audio signal";
                            break;
                        default:
                            continuity_str = ", Rfu";
                            break;
                    }
                }
                else {  // Continuity_flag == 1
                    switch (RandM) {
                        case 0x0:
                        case 0x1:
                            continuity_str = ", continuous output possible";
                            break;
                        case 0x6:
                            continuity_str = ", compensating time delay on DRM audio signal";
                            break;
                        case 0x8:
                        case 0x9:
                            continuity_str = ", compensating time delay on FM audio signal";
                            break;
                        case 0xa:
                        case 0xc:
                        case 0xe:
                            continuity_str = ", compensating time delay on AM audio signal";
                            break;
                        default:
                            continuity_str = ", Rfu";
                            break;
                    }
                }
            }
            else {  // fig0.oe() == 1
                continuity_str = ", reserved for future addition";
                r.errors.emplace_back("Rfu");
            }

            r.msgs.emplace_back(3,
                    strprintf("Continuity flag=%d ", Continuity_flag) +
                    continuity_str);

            const uint64_t key =
                ((uint64_t)fig0.oe() << 32) | ((uint64_t)fig0.pd() << 31) |
                ((uint64_t)RegionId << 20) | ((uint64_t)Id_field << 4) |
                (uint64_t)RandM;
            r.msgs.emplace_back(3,
                    strprintf("database key=0x%09" PRId64, key));

            // CEI Change Event Indication
            if (Length_Freq_list == 0) {
                r.msgs.emplace_back(3, "CEI=true");
            }

            r.msgs.emplace_back(3, "Frequency Information:");
            // Iterate over the frequency infos
            switch (RandM) {
                case 0x0:
                case 0x1:
                    {
                        // Each entry is 24 bits (5 control + 19 freq)
                        const size_t bytes_per_entry = 3;
                        const int num_freqs = Length_Freq_list / bytes_per_entry;

                        if (Length_Freq_list % bytes_per_entry != 0) {
                            r.errors.push_back("Length of freq list incorrect size");
                        }

                        for (int freq_ix = 0; freq_ix < num_freqs; freq_ix++) {
                            r.msgs.emplace_back(4, "-");
                            if (i + bytes_per_entry > fig0.figlen) {
                                r.errors.push_back(strprintf(
                                            "FIG 0/21 too small for"
                                            " FI %d, freq %d",
                                            FI_ix, freq_ix));
                                break;
                            }

                            const uint8_t Control_field = (f[i] >> 3);
                            const uint32_t freq = 16 *
                                (((uint32_t)(f[i] & 0x07) << 16) |
                                 ((uint32_t)f[i+1] << 8) |
                                 (uint32_t)f[i+2]);
                            i += bytes_per_entry;
                            if (freq == 0) {
                                r.errors.emplace_back(strprintf(
                                            "Frequency not to be used (0) in"
                                            " FI %d, freq %d",
                                            FI_ix, freq_ix));
                                continue;
                            }
                            const uint8_t Control_field_trans_mode = (Control_field >> 1) & 0x07;
                            if ((Control_field & 0x10) == 0) {
                                r.msgs.emplace_back(5,
                                        strprintf("%d KHz", freq));
                                if ((Control_field & 0x01) == 0) {
                                    r.msgs.emplace_back(5,
                                            "geographically adjacent area");
                                }
                                else {  // (Control_field & 0x01) == 1
                                    r.msgs.emplace_back(5,
                                            "no geographically adjacent area");
                                }
                                if (Control_field_trans_mode == 0) {
                                    r.msgs.emplace_back(5,
                                            "no transmission mode signalled");
                                }
                                else if (Control_field_trans_mode <= 4) {
                                    r.msgs.emplace_back(5,
                                            strprintf("transmission mode %d",
                                                Control_field_trans_mode));
                                }
                                else {  // Control_field_trans_mode > 4
                                    r.msgs.emplace_back(5,
                                            strprintf("invalid transmission mode 0x%x",
                                                Control_field_trans_mode));
                                }
                            }
                            else {  // (Control_field & 0x10) == 0x10
                                r.msgs.emplace_back(5,
                                        strprintf("%d KHz,"
                                            "invalid Control field b23 0x%x",
                                            freq, Control_field));
                            }
                        }
                    }
                    break;
                case 0x8:
                case 0x9:
                case 0xA:
                    {
                        // entries are 8-bit freq
                        const size_t bytes_per_entry = 1;
                        const int num_freqs = Length_Freq_list / bytes_per_entry;

                        for (int freq_ix = 0; freq_ix < num_freqs; freq_ix++) {
                            r.msgs.emplace_back(4, "-");
                            if (i + bytes_per_entry > fig0.figlen) {
                                r.errors.push_back(strprintf(
                                            "FIG 0/21 too small for"
                                            " FI %d, freq %d",
                                            FI_ix, freq_ix));
                            }

                            const uint8_t freq = f[i];
                            i++;
                            if (freq == 0) {
                                r.errors.emplace_back(
                                        "Frequency not to be used (0)");
                                continue;
                            }

                            if (RandM == 0xA) {
                                if (freq < 16) {
                                    r.msgs.emplace_back(5,
                                            strprintf("%d KHz",
                                                144 + ((uint32_t)freq * 9)));
                                }
                                else {  // f[k] >= 16
                                    r.msgs.emplace_back(5,
                                            strprintf("%d KHz",
                                                387 + ((uint32_t)freq * 9)));
                                }
                            }
                            else {  // RandM == 8 or 9
                                r.msgs.emplace_back(5,
                                        strprintf("%.1f MHz",
                                            87.5 + ((float)freq * 0.1)));
                            }
                        }
                    }
                    break;
                case 0xC:
                    {
                        // freqs are 16-bit
                        const size_t bytes_per_entry = 2;
                        const int num_freqs = Length_Freq_list / bytes_per_entry;

                        if (Length_Freq_list % bytes_per_entry != 0) {
                            r.errors.push_back("Length of freq list incorrect size");
                        }

                        for (int freq_ix = 0; freq_ix < num_freqs; freq_ix++) {
                            r.msgs.emplace_back(4, "-");
                            if (i + bytes_per_entry > fig0.figlen) {
                                r.errors.push_back(strprintf(
                                            "FIG 0/21 too small for"
                                            " FI %d, freq %d",
                                            FI_ix, freq_ix));
                            }

                            // freqs are 16-bit
                            const uint16_t freq = 5 *
                                (((uint16_t)f[i] << 8) |
                                 (uint32_t)f[i+1]);
                            i += bytes_per_entry;
                            if (freq != 0) {
                                r.msgs.emplace_back(5,
                                        strprintf("%d KHz", freq));
                            }
                            else {
                                r.errors.emplace_back(
                                        "Frequency not to be used (0)");
                            }
                        }
                    }
                    break;
                case 0x6:
                case 0xE:
                    {
                        // There is a first 8-bit entry, and the
                        // list contains 16-bit freqs
                        if (i + 1 > fig0.figlen) {
                            r.errors.push_back(strprintf(
                                        "FIG 0/21 too small for"
                                        " control of"
                                        " FI %d",
                                        FI_ix));
                        }

                        const size_t bytes_per_entry = 2;
                        const int num_freqs = (Length_Freq_list-1) / bytes_per_entry;

                        if ((Length_Freq_list-1) % bytes_per_entry != 0) {
                            r.errors.push_back("Length of freq list incorrect size");
                        }
                        const uint32_t Id_field2 = f[i];
                        i++;

                        for (int freq_ix = 0; freq_ix < num_freqs; freq_ix++) {
                            r.msgs.emplace_back(4, "-");
                            if (i + bytes_per_entry > fig0.figlen) {
                                r.errors.push_back(strprintf(
                                            "FIG 0/21 too small for"
                                            " FI %d, freq %d",
                                            FI_ix, freq_ix));
                            }
                            // entries are 16bit freq
                            const uint16_t freq =
                                ((((uint16_t)f[i] & 0x7f) << 8) |
                                 (uint16_t)f[i+1]);
                            i += bytes_per_entry;

                            if (freq != 0) {
                                r.msgs.emplace_back(5,
                                        strprintf("%d KHz", freq));
                            }
                            else {
                                r.errors.emplace_back(
                                        "Frequency not to be used (0)");
                            }

                            const uint32_t srv_id = (Id_field2 << 16) | Id_field;
                            if (RandM == 0x6) {
                                r.msgs.emplace_back(5,
                                        strprintf("DRM Service Id 0x%X", srv_id));
                            }
                            else if (RandM == 0xE) {
                                r.msgs.emplace_back(5,
                                        strprintf("AMSS Service Id 0x%X", srv_id));
                            }
                        }
                    }
                    break;
                default:
                    r.errors.push_back(strprintf("Invalid R&M"));
                    break;
            }
        }
    }

    return r;
}

