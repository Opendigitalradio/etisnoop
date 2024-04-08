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
#include <string>
#include <cstring>
#include <unordered_set>

static std::unordered_set<int> services_seen;

bool fig0_2_is_complete(int services_id)
{
    bool complete = services_seen.count(services_id);

    if (complete) {
        services_seen.clear();
    }

    services_seen.insert(services_id);

    return complete;
}

// FIG 0/2 Basic service and service component definition
// ETSI EN 300 401 6.3.1
fig_result_t fig0_2(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint16_t sref;
    uint32_t sid;
    uint8_t cid, ecc, local, caid, ncomp, timd, ps, ca, subchid, scty;
    int k = 1;
    uint8_t* f = fig0.f;
    fig_result_t r;

    while (k < fig0.figlen) {
        if (fig0.pd() == 0) {
            sid  = read_u16_from_buf(f + k);
            ecc  = 0;
            cid  = (f[k] & 0xF0) >> 4;
            sref = (f[k] & 0x0F) * 256uL + f[k+1];
            k += 2;
        }
        else {
            sid  = read_u32_from_buf(f + k);
            ecc  =  f[k];
            cid  = (f[k+1] & 0xF0) >> 4;
            sref = (f[k+1] & 0x0F) * 256uL * 256uL +
                   f[k+2] * 256uL +
                   f[k+3];

            k += 4;
        }

        r.complete |= fig0_2_is_complete(sid);

        local = (f[k] & 0x80) >> 7;
        caid  = (f[k] & 0x70) >> 4;
        ncomp =  f[k] & 0x0F;

        r.msgs.emplace_back(0, "-");
        r.msgs.emplace_back(1, strprintf("Service ID=0x%X", sid));
        if (fig0.pd() != 0) {
            r.msgs.emplace_back(1, strprintf("ECC=%d", ecc));
        }
        r.msgs.emplace_back(1, strprintf("Country id=%d", cid));
        r.msgs.emplace_back(1, strprintf("Service reference=%d", sref));
        r.msgs.emplace_back(1, strprintf("Number of components=%d", ncomp));
        r.msgs.emplace_back(1, strprintf("Local flag=%d", local));
        r.msgs.emplace_back(1, strprintf("CAID=%d", caid));

        if (fig0.fibcrccorrect) {
            auto& service = fig0.ensemble.get_or_create_service(sid);
            service.programme_not_data = (fig0.pd() == 0);
        }


        k++;
        r.msgs.emplace_back(1, "Components:");
        for (int i = 0; i < ncomp; i++) {
            uint8_t scomp[2];

            memcpy(scomp, f+k, 2);
            r.msgs.emplace_back(2, "-");
            r.msgs.emplace_back(3, strprintf("ID=%d", i));

            timd    = (scomp[0] & 0xC0) >> 6;
            ps      = (scomp[1] & 0x02) >> 1;
            ca      =  scomp[1] & 0x01;
            scty    =  scomp[0] & 0x3F;
            subchid = (scomp[1] & 0xFC) >> 2;

            /* useless, kept as reference
               if (timd == 3) {
               uint16_t scid;
               scid = scty*64 + subchid;
               }
               */

            if (ps == 0) {
                r.msgs.emplace_back(3, "primary=true");
            }
            else {
                r.msgs.emplace_back(3, "primary=false");
            }

            if (fig0.fibcrccorrect) {
                // TODO is subchid unique, or do we also need to take timd into the key for identifying
                // our components?
                auto& service = fig0.ensemble.get_service(sid);
                auto& component = service.get_or_create_component(subchid);
                component.primary = (ps != 0);
            }

            if (timd == 0) {
                //MSC stream audio
                r.msgs.emplace_back(3, "Mode=audio stream");

                if (scty == 0)
                    r.msgs.emplace_back(3, strprintf("ASCTy=MPEG Foreground sound (%d)", scty));
                else if (scty == 1)
                    r.msgs.emplace_back(3, strprintf("ASCTy=MPEG Background sound (%d)", scty));
                else if (scty == 2)
                    r.msgs.emplace_back(3, strprintf("ASCTy=Multi Channel sound (%d)", scty));
                else if (scty == 63)
                    r.msgs.emplace_back(3, strprintf("ASCTy=AAC sound (%d)", scty));
                else
                    r.msgs.emplace_back(3, strprintf("ASCTy=Unknown ASCTy (%d)", scty));

                r.msgs.emplace_back(3, strprintf("SubChannel ID=0x%02X", subchid));
                r.msgs.emplace_back(3, strprintf("CA=%d", ca));
            }
            else if (timd == 1) {
                // MSC stream data
                r.msgs.emplace_back(3, "Mode=data stream");
                r.msgs.emplace_back(3, strprintf("DSCTy=%d %s", scty, get_dscty_type(scty)));
                r.msgs.emplace_back(3, strprintf("SubChannel ID=0x%02X", subchid));
                r.msgs.emplace_back(3, strprintf("CA=%d", ca));
            }
            else if (timd == 2) {
                // FIDC
                r.msgs.emplace_back(3, "Mode=FIDC");
                r.msgs.emplace_back(3, strprintf("DSCTy=%d %s", scty, get_dscty_type(scty)));
                r.msgs.emplace_back(3, strprintf("Fast Information Data Channel ID=0x%02X", subchid));
                r.msgs.emplace_back(3, strprintf("CA=%d", ca));
            }
            else if (timd == 3) {
                // MSC Packet mode
                r.msgs.emplace_back(3, "Mode=MSC Packet");
                r.msgs.emplace_back(3, strprintf("SubChannel ID=0x%02X", subchid));
                r.msgs.emplace_back(3, strprintf("CA=%d", ca));
            }

            k += 2;
        }
    }

    return r;
}

