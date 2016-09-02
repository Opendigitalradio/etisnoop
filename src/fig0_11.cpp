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
bool fig0_11(fig0_common_t& fig0, int indent)
{
    Lat_Lng gps_pos = {0, 0};
    int16_t Latitude_coarse, Longitude_coarse;
    uint16_t Region_Id, Extent_Latitude, Extent_Longitude, key;
    uint8_t i = 1, j, k, GATy, Rfu, Length_TII_list, Rfa, MainId, Length_SubId_list, SubId;
    int8_t bit_pos;
    char desc[256];
    char tmpbuf[256];
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
            sprintf(desc, "GATy=%d Geographical area defined by a TII list, G/E flag=%d %s coverage area, RegionId=0x%X, database key=0x%X",
                    GATy, GE_flag, GE_flag?"Global":"Ensemble", Region_Id, key);
            if (i < fig0.figlen) {
                Rfu = f[i] >> 5;
                if (Rfu != 0) {
                    sprintf(tmpbuf, ", Rfu=%d invalid value", Rfu);
                    strcat(desc, tmpbuf);
                }
                Length_TII_list = f[i] & 0x1F;
                sprintf(tmpbuf, ", Length of TII list=%d", Length_TII_list);
                strcat(desc, tmpbuf);
                if (Length_TII_list == 0) {
                    strcat(desc, ", CEI");
                }
                printbuf(desc, indent+1, NULL, 0);
                i++;

                for(j = 0;(i < (fig0.figlen - 1)) && (j < Length_TII_list); j++) {
                    // iterate over Transmitter group
                    Rfa = f[i] >> 7;
                    MainId = f[i] & 0x7F;
                    if (Rfa != 0) {
                        sprintf(desc, "Rfa=%d invalid value, MainId=0x%X",
                                Rfa, MainId);
                    }
                    else {
                        sprintf(desc, "MainId=0x%X", MainId);
                    }
                    // check MainId value
                    if ((Mode_Identity == 1) || (Mode_Identity == 2) || (Mode_Identity == 4)) {
                        if (MainId > 69) {
                            // The coding range shall be 0 to 69 for transmission modes I, II and IV
                            sprintf(tmpbuf, " invalid value for transmission mode %d", Mode_Identity);
                            strcat(desc, tmpbuf);
                        }
                    }
                    else if (Mode_Identity == 3) {
                        if (MainId > 5) {
                            // The coding range shall be 0 to 5 for transmission modes I, II and IV
                            sprintf(tmpbuf, " invalid value for transmission mode %d", Mode_Identity);
                            strcat(desc, tmpbuf);
                        }
                    }
                    Rfa = f[i+1] >> 5;
                    if (Rfa != 0) {
                        sprintf(tmpbuf, ", Rfa=%d invalid value", Rfa);
                        strcat(desc, tmpbuf);
                    }
                    Length_SubId_list = f[i+1] & 0x1F;
                    sprintf(tmpbuf, ", Length of SubId=%d", Length_SubId_list);
                    strcat(desc, tmpbuf);
                    printbuf(desc, indent+2, NULL, 0);
                    i += 2;

                    bit_pos = 3;
                    SubId = 0;
                    for(k = 0;(i < fig0.figlen) && (k < Length_SubId_list); k++) {
                        // iterate SubId
                        if (bit_pos >= 0) {
                            SubId |= (f[i] >> bit_pos) & 0x1F;
                            sprintf(desc, "SubId=0x%X", SubId);
                            // check SubId value
                            if ((SubId == 0) || (SubId > 23)) {
                                strcat(desc, " invalid value");
                            }
                            printbuf(desc, indent+3, NULL, 0);
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
                        sprintf(desc, "%d SubId missing, fig length too short !", (Length_SubId_list - k));
                        printbuf(desc, indent+3, NULL, 0);
                        fprintf(stderr, "WARNING: FIG 0/%d length %d too short !\n", fig0.ext(), fig0.figlen);
                    }
                }
                if (j < Length_TII_list) {
                    sprintf(desc, "%d Transmitter group missing, fig length too short !", (Length_TII_list - j));
                    printbuf(desc, indent+2, NULL, 0);
                    fprintf(stderr, "WARNING: FIG 0/%d length %d too short !\n", fig0.ext(), fig0.figlen);
                }
            }
        }
        else if (GATy == 1) {
            // Coordinates
            sprintf(desc, "GATy=%d Geographical area defined as a spherical rectangle by the geographical co-ordinates of one corner and its latitude and longitude extents, G/E flag=%d %s coverage area, RegionId=0x%X, database key=0x%X",
                    GATy, GE_flag, GE_flag?"Global":"Ensemble", Region_Id, key);
            if (i < (fig0.figlen - 6)) {
                Latitude_coarse = ((int16_t)f[i] << 8) | ((uint16_t)f[i+1]);
                Longitude_coarse = ((int16_t)f[i+2] << 8) | ((uint16_t)f[i+3]);
                gps_pos.latitude = ((double)Latitude_coarse) * 90 / 32768;
                gps_pos.longitude = ((double)Latitude_coarse) * 180 / 32768;
                sprintf(tmpbuf, ", Lat Lng coarse=0x%X 0x%X => %f, %f",
                        Latitude_coarse, Longitude_coarse, gps_pos.latitude, gps_pos.longitude);
                strcat(desc, tmpbuf);
                Extent_Latitude = ((uint16_t)f[i+4] << 4) | ((uint16_t)(f[i+5] >> 4));
                Extent_Longitude = ((uint16_t)(f[i+5] & 0x0F) << 8) | ((uint16_t)f[i+6]);
                gps_pos.latitude += ((double)Extent_Latitude) * 90 / 32768;
                gps_pos.longitude += ((double)Extent_Longitude) * 180 / 32768;
                sprintf(tmpbuf, ", Extent Lat Lng=0x%X 0x%X => %f, %f",
                        Extent_Latitude, Extent_Longitude, gps_pos.latitude, gps_pos.longitude);
                strcat(desc, tmpbuf);
            }
            else {
                sprintf(tmpbuf, ", Coordinates missing, fig length too short !");
                strcat(desc, tmpbuf);
                fprintf(stderr, "WARNING: FIG 0/%d length %d too short !\n", fig0.ext(), fig0.figlen);
            }
            printbuf(desc, indent+1, NULL, 0);
            i += 7;
        }
        else {
            // Rfu
            sprintf(desc, "GATy=%d reserved for future use of the geographical, G/E flag=%d %s coverage area, RegionId=0x%X, database key=0x%X, stop Region definition iteration %d/%d",
                    GATy, GE_flag, GE_flag?"Global":"Ensemble", Region_Id, key, i, fig0.figlen);
            printbuf(desc, indent+1, NULL, 0);
            // stop Region definition iteration
            i = fig0.figlen;
        }
    }

    return complete;
}

