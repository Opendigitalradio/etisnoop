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

// FIG 0/0 Ensemble information
// ETSI EN 300 401 6.4
fig_result_t fig0_0(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint8_t occ;
    fig_result_t r;
    uint8_t* f = fig0.f;

    const uint16_t eid = read_u16_from_buf(f + 1);
    r.msgs.push_back(strprintf("Ensemble ID=0x%02x", eid));
    if (fig0.fibcrccorrect) {
        fig0.ensemble.EId = eid;
    }

    const uint8_t cid  = (f[1] & 0xF0) >> 4;
    r.msgs.emplace_back(strprintf("Country ID=%d", cid));

    const uint16_t eref = (f[1] & 0x0F)*256 + \
                           f[2];
    r.msgs.emplace_back(strprintf("Ensemble reference=%d", eref));

    const uint8_t ch = (f[3] & 0xC0) >> 6;
    r.msgs.push_back(strprintf("Change flag=%d", ch));

    const uint8_t al = (f[3] & 0x20) >> 5;
    r.msgs.push_back(strprintf("Alarm flag=%d", al));

    const uint8_t hic = f[3] & 0x1F;
    const uint8_t lowc = f[4];
    r.msgs.push_back(strprintf("CIF Count=%d/%d", hic, lowc));

    if (ch != 0) {
        occ = f[5];
        r.msgs.push_back(strprintf("Occurrence change=%d", occ));
    }

    r.complete = true;

    return r;
}

