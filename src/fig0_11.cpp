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

static std::unordered_set<int> region_ids_seen;

bool fig0_11_is_complete(int region_id)
{
    bool complete = region_ids_seen.count(region_id);

    if (complete) {
        region_ids_seen.clear();
    }
    else {
        region_ids_seen.insert(region_id);
    }

    return complete;
}


// FIG 0/11 Region definition
// ETSI EN 300 401 8.1.16.1
fig_result_t fig0_11(fig0_common_t& fig0, const display_settings_t &disp)
{
    Lat_Lng gps_pos = {0, 0};
    int16_t Latitude_coarse, Longitude_coarse;
    uint16_t Region_Id, Extent_Latitude, Extent_Longitude, key;
    uint8_t i = 1, j, k, GATy, Rfu, Length_TII_list, Rfa, MainId, Length_SubId_list, SubId;
    int8_t bit_pos;
    fig_result_t r;
    bool GE_flag;
    uint8_t* f = fig0.f;
    uint8_t Mode_Identity = get_mode_identity();
    bool complete = false;

    while (i < (fig0.figlen - 1)) {
        // iterate over Region definition
        GATy = f[i] >> 4;
        GE_flag = (f[i] >> 3) & 0x01;
        Region_Id = ((uint16_t)(f[i] & 0x07) << 8) | ((uint16_t)f[i+1]);
        complete |= fig0_11_is_complete(Region_Id);

        key = ((uint16_t)fig0.oe() << 12) | ((uint16_t)fig0.pd() << 11) | Region_Id;
        i += 2;
        if (GATy == 0) {
            // TII list
            r.msgs.push_back(strprintf("GATy=%d", GATy));
            r.msgs.emplace_back("Geographical area defined by a TII list");
            r.msgs.push_back(strprintf("G/E flag=%d %s coverage area",
                        GE_flag, GE_flag ? "Global" : "Ensemble"));
            r.msgs.push_back(strprintf("RegionId=0x%X", Region_Id));
            r.msgs.push_back(strprintf("database key=0x%X", key));

            if (i < fig0.figlen) {
                Rfu = f[i] >> 5;
                if (Rfu != 0) {
                    r.errors.push_back(strprintf("Rfu=%d invalid value", Rfu));
                }
                Length_TII_list = f[i] & 0x1F;
                r.msgs.push_back(strprintf(", Length of TII list=%d", Length_TII_list));
                if (Length_TII_list == 0) {
                    r.msgs.emplace_back("CEI");
                }
                i++;

                for (j = 0; (i < (fig0.figlen - 1)) && (j < Length_TII_list); j++) {
                    // iterate over Transmitter group
                    Rfa = f[i] >> 7;
                    MainId = f[i] & 0x7F;
                    if (Rfa != 0) {
                        r.errors.push_back(strprintf("Rfa=%d invalid value, MainId=0x%X", Rfa, MainId));
                    }
                    else {
                        r.msgs.emplace_back(1, strprintf("MainId=0x%X", MainId));
                    }
                    // check MainId value
                    if ((Mode_Identity == 1) || (Mode_Identity == 2) || (Mode_Identity == 4)) {
                        if (MainId > 69) {
                            // The coding range shall be 0 to 69 for transmission modes I, II and IV
                            r.errors.push_back(strprintf("invalid value for transmission mode %d", Mode_Identity));
                        }
                    }
                    else if (Mode_Identity == 3) {
                        if (MainId > 5) {
                            // The coding range shall be 0 to 5 for transmission modes I, II and IV
                            r.errors.push_back(strprintf("invalid value for transmission mode %d", Mode_Identity));
                        }
                    }
                    Rfa = f[i+1] >> 5;
                    if (Rfa != 0) {
                        r.errors.push_back(strprintf("Rfa=%d invalid value", Rfa));
                    }
                    Length_SubId_list = f[i+1] & 0x1F;
                    r.msgs.emplace_back(1, strprintf("Length of SubId=%d", Length_SubId_list));
                    i += 2;

                    bit_pos = 3;
                    SubId = 0;
                    for(k = 0;(i < fig0.figlen) && (k < Length_SubId_list); k++) {
                        // iterate SubId
                        if (bit_pos >= 0) {
                            SubId |= (f[i] >> bit_pos) & 0x1F;
                            r.msgs.emplace_back(2, strprintf("SubId=0x%X", SubId));
                            // check SubId value
                            if ((SubId == 0) || (SubId > 23)) {
                                r.errors.push_back(strprintf("Invalid SubId=0x%X", SubId));
                            }
                            bit_pos -= 5;
                            SubId = 0;
                        }
                        if (bit_pos < 0) {
                            SubId = (f[i] << abs(bit_pos)) & 0x1F;
                            bit_pos += 8;
                            i++;
                        }
                    }
                    if (bit_pos > 3) {
                        // jump padding
                        i++;
                    }
                    if (k < Length_SubId_list) {
                        r.errors.push_back(strprintf("%d SubId missing, fig length too short !", Length_SubId_list - k));
                    }
                }
                if (j < Length_TII_list) {
                    r.errors.push_back(strprintf("%d Transmitter group missing, fig length too short !", Length_TII_list - j));
                }
            }
        }
        else if (GATy == 1) {
            // Coordinates
            r.msgs.push_back(strprintf("GATy=%d", GATy));
            r.msgs.emplace_back("Geographical area defined as a spherical rectangle "
                    "by the geographical co-ordinates of one corner and its latitude and "
                    "longitude extents");
            r.msgs.push_back(strprintf("G/E flag=%d %s coverage area",
                    GE_flag, GE_flag ? "Global" : "Ensemble"));
            r.msgs.push_back(strprintf("RegionId=0x%X", Region_Id));
            r.msgs.push_back(strprintf("database key=0x%X", key));

            if (i < (fig0.figlen - 6)) {
                Latitude_coarse = ((int16_t)f[i] << 8) | ((uint16_t)f[i+1]);
                Longitude_coarse = ((int16_t)f[i+2] << 8) | ((uint16_t)f[i+3]);
                gps_pos.latitude = ((double)Latitude_coarse) * 90 / 32768;
                gps_pos.longitude = ((double)Latitude_coarse) * 180 / 32768;
                r.msgs.push_back(strprintf("Lat Lng coarse=0x%X 0x%X => %f, %f",
                        Latitude_coarse, Longitude_coarse, gps_pos.latitude, gps_pos.longitude));
                Extent_Latitude = ((uint16_t)f[i+4] << 4) | ((uint16_t)(f[i+5] >> 4));
                Extent_Longitude = ((uint16_t)(f[i+5] & 0x0F) << 8) | ((uint16_t)f[i+6]);
                gps_pos.latitude += ((double)Extent_Latitude) * 90 / 32768;
                gps_pos.longitude += ((double)Extent_Longitude) * 180 / 32768;
                r.msgs.push_back(strprintf("Extent Lat Lng=0x%X 0x%X => %f, %f",
                        Extent_Latitude, Extent_Longitude, gps_pos.latitude, gps_pos.longitude));
            }
            else {
                r.errors.push_back("Coordinates missing, fig length too short !");
            }
            i += 7;
        }
        else {
            // Rfu
            r.msgs.push_back(strprintf("GATy=%d", GATy));
            r.msgs.emplace_back("reserved for future use of the geographical");
            r.msgs.push_back(strprintf("G/E flag=%d %s coverage area",
                        GE_flag, GE_flag ? "Global" : "Ensemble"));
            r.msgs.push_back(strprintf("RegionId=0x%X", Region_Id));
            r.msgs.push_back(strprintf("database key=0x%X", key));
            r.msgs.push_back(strprintf("stop Region definition iteration %d/%d",
                     i, fig0.figlen));
            // stop Region definition iteration
            i = fig0.figlen;
            r.errors.push_back("Stopping iteration because Rfu encountered");
        }
    }

    r.complete = complete;
    return r;
}

