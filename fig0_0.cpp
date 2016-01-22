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

// FIG 0/0 Ensemble information
// ETSI EN 300 401 6.4
bool fig0_0(fig0_common_t& fig0, int indent)
{
    uint8_t cid, al, ch, hic, lowc, occ;
    uint16_t eid, eref;
    char desc[128];
    uint8_t* f = fig0.f;

    eid  =  f[1]*256+f[2];
    cid  = (f[1] & 0xF0) >> 4;
    eref = (f[1] & 0x0F)*256 + \
            f[2];
    ch   = (f[3] & 0xC0) >> 6;
    al   = (f[3] & 0x20) >> 5;
    hic  =  f[3] & 0x1F;
    lowc =  f[4];
    if (ch != 0) {
        occ = f[5];
        sprintf(desc,
                "Ensemble ID=0x%02x (Country id=%d, Ensemble reference=%d), Change flag=%d, Alarm flag=%d, CIF Count=%d/%d, Occurance change=%d",
                eid, cid, eref, ch, al, hic, lowc, occ);
    }
    else {
        sprintf(desc,
                "Ensemble ID=0x%02x (Country id=%d, Ensemble reference=%d), Change flag=%d, Alarm flag=%d, CIF Count=%d/%d",
                eid, cid, eref, ch, al, hic, lowc);
    }
    printbuf(desc, indent+1, NULL, 0);

    return true;
}

