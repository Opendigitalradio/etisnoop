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

bool fig0_28_is_complete(int cluster_id)
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


// FIG 0/28 FM Announcement switching
// ETSI EN 300 401 8.1.11.2.2
bool fig0_28(fig0_common_t& fig0, int indent)
{
    uint16_t PI;
    uint8_t i = 1, Cluster_Id_Current_Ensemble, Region_Id_Current_Ensemble;
    bool New_flag, Rfa;
    char tmpbuf[256];
    char desc[256];
    uint8_t* f = fig0.f;
    bool complete = false;

    while (i < (fig0.figlen - 3)) {
        // iterate over FM announcement switching
        Cluster_Id_Current_Ensemble = f[i];
        complete = fig0_28_is_complete(Cluster_Id_Current_Ensemble);
        New_flag = f[i+1] >> 7;
        Rfa = (f[i+1] >> 6) & 0x01;
        Region_Id_Current_Ensemble = f[i+1] & 0x3F;
        PI = ((uint16_t)f[i+2] << 8) | (uint16_t)f[i+3];
        sprintf(desc, "Cluster Id Current Ensemble=0x%X", Cluster_Id_Current_Ensemble);
        if (Cluster_Id_Current_Ensemble == 0) {
            strcat(desc, " invalid value");
            fprintf(stderr, "WARNING: FIG 0/%d Cluster Id Current Ensemble invalid value 0\n", fig0.ext());
        }
        sprintf(tmpbuf, ", New flag=%d %s announcement",
                New_flag, New_flag?"newly introduced":"repeated");
        strcat(desc, tmpbuf);
        if (Rfa != 0) {
            sprintf(tmpbuf, ", Rfa=%d invalid value", Rfa);
            strcat(desc, tmpbuf);
        }
        sprintf(tmpbuf, ", Region Id Current Ensemble=0x%X, PI=0x%X", Region_Id_Current_Ensemble, PI);
        strcat(desc, tmpbuf);
        printbuf(desc, indent+1, NULL, 0);
        i += 4;
    }

    return complete;
}

