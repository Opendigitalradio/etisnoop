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
bool fig0_22(fig0_common_t& fig0, int indent)
{
    Lat_Lng gps_pos = {0, 0};
    double latitude_sub, longitude_sub;
    int16_t Latitude_coarse, Longitude_coarse;
    uint16_t key, TD;
    int16_t Latitude_offset, Longitude_offset;
    uint8_t i = 1, j, MainId = 0, Rfu, Nb_SubId_fields, SubId;
    uint8_t Latitude_fine, Longitude_fine;
    char tmpbuf[256];
    char desc[256];
    bool MS;
    const uint8_t Mode_Identity = get_mode_identity();
    uint8_t* f = fig0.f;
    bool complete = false;

    while (i < fig0.figlen) {
        // iterate over Transmitter Identification Information (TII) fields
        MS = f[i] >> 7;
        MainId = f[i] & 0x7F;
        complete |= fig0_22_is_complete(MS, MainId);
        key = (fig0.oe() << 8) | (fig0.pd() << 7) | MainId;
        sprintf(desc, "M/S=%d %sidentifier, MainId=0x%X",
                MS, MS?"Sub-":"Main ", MainId);
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
        // print database key
        sprintf(tmpbuf, ", database key=0x%X", key);
        strcat(desc, tmpbuf);
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
                sprintf(tmpbuf, ", Lat Lng coarse=0x%X 0x%X, Lat Lng fine=0x%X 0x%X => Lat Lng=%f, %f",
                        Latitude_coarse, Longitude_coarse, Latitude_fine, Longitude_fine,
                        gps_pos.latitude, gps_pos.longitude);
                strcat(desc, tmpbuf);
                i += 5;
            }
            else {
                strcat(desc, ", invalid length of Latitude Longitude coarse fine");
            }
            printbuf(desc, indent+1, NULL, 0);
        }
        else {  // MS == 1
            // Sub-identifier

            if (i < fig0.figlen) {
                Rfu = f[i] >> 3;
                Nb_SubId_fields = f[i] & 0x07;
                if (Rfu != 0) {
                    sprintf(tmpbuf, ", Rfu=%d invalid value", Rfu);
                    strcat(desc, tmpbuf);
                }
                sprintf(tmpbuf, ", Number of SubId fields=%d%s",
                        Nb_SubId_fields, (Nb_SubId_fields == 0)?", CEI":"");
                strcat(desc, tmpbuf);
                printbuf(desc, indent+1, NULL, 0);
                i++;

                for(j = i; ((j < (i + (Nb_SubId_fields * 6))) && (j < (fig0.figlen - 5))); j += 6) {
                    // iterate over SubId fields
                    SubId = f[j] >> 3;
                    sprintf(desc, "SubId=0x%X", SubId);
                    // check SubId value
                    if ((SubId == 0) || (SubId > 23)) {
                        strcat(desc, " invalid value");
                    }

                    TD = ((f[j] & 0x03) << 8) | f[j+1];
                    Latitude_offset = (f[j+2] << 8) | f[j+3];
                    Longitude_offset = (f[j+4] << 8) | f[j+5];
                    sprintf(tmpbuf, ", TD=%d us, Lat Lng offset=0x%X 0x%X",
                            TD, Latitude_offset, Longitude_offset);
                    strcat(desc, tmpbuf);

                    if (fig0_22_key_Lat_Lng.count(key) > 0) {
                        // latitude longitude available in database for Main Identifier
                        latitude_sub = (90 * (double)Latitude_offset / 524288) + fig0_22_key_Lat_Lng[key].latitude;
                        longitude_sub = (180 * (double)Longitude_offset / 524288) + fig0_22_key_Lat_Lng[key].longitude;
                        sprintf(tmpbuf, " => Lat Lng=%f, %f", latitude_sub, longitude_sub);
                        strcat(desc, tmpbuf);
                    }
                    else {
                        // latitude longitude not available in database for Main Identifier
                        latitude_sub = 90 * (double)Latitude_offset / 524288;
                        longitude_sub = 180 * (double)Longitude_offset / 524288;
                        sprintf(tmpbuf, " => Lat Lng=%f, %f wrong value because Main identifier latitude/longitude not available in database", latitude_sub, longitude_sub);
                        strcat(desc, tmpbuf);
                    }
                    printbuf(desc, indent+2, NULL, 0);
                }
                i += (Nb_SubId_fields * 6);
            }
            else {
                strcat(desc, ", invalid fig length or Number of SubId fields length");
                printbuf(desc, indent+1, NULL, 0);
            }
        }
    }

    return complete;
}

