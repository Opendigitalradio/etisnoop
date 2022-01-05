/*
    Copyright (C) 2020 Matthias P. Braendli (http://www.opendigitalradio.org)
    Copyright (C) 2015 Data Path
    Copyright (C) 2014 CSP Innovazione nelle ICT s.c.a r.l. (http://www.csp.it/)

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


// FIG 0/7 Configuration Information
// ETSI EN 300 401 v2.1.1 Clause 6.4.2
fig_result_t fig0_7(fig0_common_t& fig0, const display_settings_t &disp)
{
    fig_result_t r;

    if (fig0.figlen != 3) {
        r.errors.push_back("FIG0/7 has incorrect length");
    }
    else {
        const uint8_t *f = fig0.f;

        const uint16_t service_count = read_u16_from_buf(f + 1);

        const uint8_t services = service_count >> 10;
        const uint16_t count = service_count & 0x3FF;

        r.msgs.push_back(strprintf("Services=%d", services));
        r.msgs.push_back(strprintf("Count=%d", count));
    }

    r.complete = true;
    return r;
}

