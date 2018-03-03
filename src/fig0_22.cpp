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

static std::unordered_set<int> identifiers_seen;

bool fig0_22_is_complete(int M_S, int MainId)
{
    int identifier = (M_S << 7) | MainId;

    bool complete = identifiers_seen.count(identifier);

    if (complete) {
        identifiers_seen.clear();
    }
    else {
        identifiers_seen.insert(identifier);
    }

    return complete;
}


// map for fig 0/22 database
std::map<uint16_t, Lat_Lng> fig0_22_key_Lat_Lng;

void fig0_22_cleardb()
{
    fig0_22_key_Lat_Lng.clear();
}

// FIG 0/22 Transmitter Identification Information (TII) database
// ETSI EN 300 401 8.1.9
fig_result_t fig0_22(fig0_common_t& fig0, const display_settings_t &disp)
{
    Lat_Lng gps_pos = {0, 0};
    double latitude_sub, longitude_sub;
    int16_t Latitude_coarse, Longitude_coarse;
    uint16_t key, TD;
    int16_t Latitude_offset, Longitude_offset;
    uint8_t i = 1, j, MainId = 0, Rfu, Nb_SubId_fields, SubId;
    uint8_t Latitude_fine, Longitude_fine;
    fig_result_t r;
    bool MS;
    const uint8_t Mode_Identity = get_mode_identity();
    uint8_t* f = fig0.f;

    while (i < fig0.figlen) {
        // iterate over Transmitter Identification Information (TII) fields
        MS = f[i] >> 7;
        MainId = f[i] & 0x7F;
        r.complete |= fig0_22_is_complete(MS, MainId);
        key = (fig0.oe() << 8) | (fig0.pd() << 7) | MainId;
        r.msgs.emplace_back("-");
        r.msgs.emplace_back(1, strprintf("M/S=%d %sidentifier",
                    MS, MS?"Sub-":"Main "));
        r.msgs.emplace_back(1, strprintf("MainId=0x%X", MainId));
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
        // print database key
        r.msgs.emplace_back(1, strprintf("database key=0x%X", key));
        i++;
        if (MS == 0) {
            // Main identifier

            if (i < (fig0.figlen - 4)) {
                Latitude_coarse = (f[i] << 8) | f[i+1];
                Longitude_coarse = (f[i+2] << 8) | f[i+3];
                Latitude_fine = f[i+4] >> 4;
                Longitude_fine = f[i+4] & 0x0F;
                gps_pos.latitude = (double)((int32_t)((((int32_t)Latitude_coarse) << 4) | (uint32_t)Latitude_fine)) * 90 / 524288;
                gps_pos.longitude = (double)((int32_t)((((int32_t)Longitude_coarse) << 4) | (uint32_t)Longitude_fine)) * 180 / 524288;
                fig0_22_key_Lat_Lng[key] = gps_pos;
                r.msgs.emplace_back(1, strprintf("Lat Lng coarse=0x%X 0x%X, Lat Lng fine=0x%X 0x%X => Lat Lng=%f, %f",
                        Latitude_coarse, Longitude_coarse, Latitude_fine, Longitude_fine,
                        gps_pos.latitude, gps_pos.longitude));
                i += 5;
            }
            else {
                r.errors.push_back("invalid length of Latitude Longitude coarse fine");
            }
        }
        else {  // MS == 1
            // Sub-identifier

            if (i < fig0.figlen) {
                Rfu = f[i] >> 3;
                Nb_SubId_fields = f[i] & 0x07;
                if (Rfu != 0) {
                    r.errors.push_back(strprintf("Rfu=%d invalid value", Rfu));
                }
                r.msgs.emplace_back(1, strprintf("Number of SubId fields=%d%s",
                        Nb_SubId_fields, (Nb_SubId_fields == 0)?", CEI":""));
                i++;

                r.msgs.emplace_back(1, "SubId Fields:");
                for(j = i; ((j < (i + (Nb_SubId_fields * 6))) && (j < (fig0.figlen - 5))); j += 6) {
                    // iterate over SubId fields
                    SubId = f[j] >> 3;
                    r.msgs.emplace_back(2, "-");
                    r.msgs.emplace_back(3, strprintf("SubId=0x%X", SubId));
                    // check SubId value
                    if ((SubId == 0) || (SubId > 23)) {
                        r.errors.push_back("invalid value");
                    }

                    TD = ((f[j] & 0x03) << 8) | f[j+1];
                    Latitude_offset = (f[j+2] << 8) | f[j+3];
                    Longitude_offset = (f[j+4] << 8) | f[j+5];
                    r.msgs.emplace_back(3, strprintf("TD=%d us", TD));
                    r.msgs.emplace_back(3, strprintf("Lat Lng offset=0x%X 0x%X", Latitude_offset, Longitude_offset));

                    if (fig0_22_key_Lat_Lng.count(key) > 0) {
                        // latitude longitude available in database for Main Identifier
                        latitude_sub = (90 * (double)Latitude_offset / 524288) + fig0_22_key_Lat_Lng[key].latitude;
                        longitude_sub = (180 * (double)Longitude_offset / 524288) + fig0_22_key_Lat_Lng[key].longitude;
                        r.msgs.emplace_back(3, strprintf("Lat Lng=%f, %f", latitude_sub, longitude_sub));
                    }
                    else {
                        // latitude longitude not available in database for Main Identifier
                        latitude_sub = 90 * (double)Latitude_offset / 524288;
                        longitude_sub = 180 * (double)Longitude_offset / 524288;
                        r.msgs.emplace_back(3, strprintf("Lat Lng=%f, %f wrong value because"
                                    " Main identifier latitude/longitude not available in database", latitude_sub, longitude_sub));
                    }
                }
                i += (Nb_SubId_fields * 6);
            }
            else {
                r.errors.emplace_back(strprintf("invalid fig length or Number of SubId fields length"));
            }
        }
    }

    return r;
}

