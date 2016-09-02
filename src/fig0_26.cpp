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

static std::unordered_set<int> clusters_seen;

bool fig0_26_is_complete(int cluster_id)
{
    bool complete = clusters_seen.count(cluster_id);

    if (complete) {
        clusters_seen.clear();
    }
    else {
        clusters_seen.insert(cluster_id);
    }

    return complete;
}


// FIG 0/26 fig0.oe() Announcement switching
// ETSI EN 300 401 8.1.10.5.2
bool fig0_26(fig0_common_t& fig0, int indent)
{
    uint16_t Asw_flags, EId_Other_Ensemble;
    uint8_t i = 1, j, Rfa, Cluster_Id_Current_Ensemble, Region_Id_Current_Ensemble;
    uint8_t Cluster_Id_Other_Ensemble, Region_Id_Other_Ensemble;
    bool New_flag, Region_flag;
    char tmpbuf[256];
    char desc[256];
    uint8_t* f = fig0.f;
    bool complete = false;

    while (i < (fig0.figlen - 6)) {
        // iterate over other ensembles announcement switching
        Cluster_Id_Current_Ensemble = f[i];
        complete = fig0_26_is_complete(Cluster_Id_Current_Ensemble);
        Asw_flags = ((uint16_t)f[i+1] << 8) | (uint16_t)f[i+2];
        New_flag = f[i+3] >> 7;
        Region_flag = (f[i+3] >> 6) & 0x01;
        Region_Id_Current_Ensemble = f[i+3] & 0x3F;
        EId_Other_Ensemble = ((uint16_t)f[i+4] << 8) | (uint16_t)f[i+5];
        Cluster_Id_Other_Ensemble = f[i+6];
        sprintf(desc, "Cluster Id Current Ensemble=0x%X, Asw flags=0x%X, New flag=%d %s announcement, Region flag=%d last byte %s, Region Id Current Ensemble=0x%X, EId Other Ensemble=0x%X, Cluster Id Other Ensemble=0x%X",
                Cluster_Id_Current_Ensemble, Asw_flags, New_flag, New_flag?"newly introduced":"repeated",
                Region_flag, Region_flag?"present":"absent. The announcement concerns the whole service area",
                Region_Id_Current_Ensemble, EId_Other_Ensemble, Cluster_Id_Other_Ensemble);
        i += 7;
        if (Region_flag != 0) {
            if (i < fig0.figlen) {
                // get Region Id Other Ensemble
                Rfa = (f[i] >> 6);
                Region_Id_Other_Ensemble = f[i] & 0x3F;
                if (Rfa != 0) {
                    sprintf(tmpbuf, ", Rfa=%d invalid value", Rfa);
                    strcat(desc, tmpbuf);
                }
                sprintf(tmpbuf, ", Region Id Other Ensemble=0x%X", Region_Id_Other_Ensemble);
                strcat(desc, tmpbuf);
            }
            else {
                sprintf(tmpbuf, "missing Region Id Other Ensemble, fig length too short !");
                strcat(desc, tmpbuf);
                fprintf(stderr, "WARNING: FIG 0/%d length %d too short !\n", fig0.ext(), fig0.figlen);
            }
            i++;
        }
        printbuf(desc, indent+1, NULL, 0);
        // decode announcement switching types
        for(j = 0; j < 16; j++) {
            if (Asw_flags & (1 << j)) {
                sprintf(desc, "Announcement switching=%s", get_announcement_type(j));
                printbuf(desc, indent+2, NULL, 0);
            }
        }
    }

    return complete;
}

