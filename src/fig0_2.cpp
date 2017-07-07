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
    else {
        services_seen.insert(services_id);
    }

    return complete;
}

// FIG 0/2 Basic service and service component definition
// ETSI EN 300 401 6.3.1
fig_result_t fig0_2(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint16_t sref, sid;
    uint8_t cid, ecc, local, caid, ncomp, timd, ps, ca, subchid, scty;
    int k = 1;
    std::string psdesc;
    uint8_t* f = fig0.f;
    fig_result_t r;

    while (k < fig0.figlen) {
        if (fig0.pd() == 0) {
            sid  =  f[k] * 256 + f[k+1];
            ecc  = 0;
            cid  = (f[k] & 0xF0) >> 4;
            sref = (f[k] & 0x0F) * 256 + f[k+1];
            k += 2;
        }
        else {
            sid  =  f[k] * 256 * 256 * 256 + \
                    f[k+1] * 256 * 256 + \
                    f[k+2] * 256 + \
                    f[k+3];

            ecc  =  f[k];
            cid  = (f[k+1] & 0xF0) >> 4;
            sref = (f[k+1] & 0x0F) * 256 * 256 + \
                   f[k+2] * 256 + \
                   f[k+3];

            k += 4;
        }

        r.complete |= fig0_2_is_complete(sid);

        local = (f[k] & 0x80) >> 7;
        caid  = (f[k] & 0x70) >> 4;
        ncomp =  f[k] & 0x0F;

        if (fig0.pd() == 0) {
            r.msgs.push_back(strprintf("Service ID=0x%X (Country id=%d, Service reference=%d)",
                        sid, cid, sref));
            r.msgs.emplace_back(1, strprintf("Number of components=%d", ncomp));
            r.msgs.emplace_back(1, strprintf("Local flag=%d", local));
            r.msgs.emplace_back(1, strprintf("CAID=%d", caid));
        }
        else {
            r.msgs.push_back(strprintf("Service ID=0x%X (ECC=%d, Country id=%d, Service reference=%d)",
                        sid, ecc, cid, sref));
            r.msgs.emplace_back(1, strprintf("Number of components=%d", ncomp));
            r.msgs.emplace_back(1, strprintf("Local flag=%d", local));
            r.msgs.emplace_back(1, strprintf("CAID=%d", caid));
        }

        if (fig0.fibcrccorrect) {
            auto& service = fig0.ensemble.get_or_create_service(sid);
            service.programme_not_data = (fig0.pd() == 0);
        }


        k++;
        for (int i = 0; i < ncomp; i++) {
            uint8_t scomp[2];

            memcpy(scomp, f+k, 2);
            r.msgs.emplace_back(1, strprintf("Component[%d]", i));

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
                psdesc = "Secondary service";
            }
            else {
                psdesc = "Primary service";
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
                r.msgs.emplace_back(2, "Stream audio mode");
                r.msgs.emplace_back(2, psdesc);

                if (scty == 0)
                    r.msgs.emplace_back(2, strprintf("MPEG Foreground sound (%d)", scty));
                else if (scty == 1)
                    r.msgs.emplace_back(2, strprintf("MPEG Background sound (%d)", scty));
                else if (scty == 2)
                    r.msgs.emplace_back(2, strprintf("Multi Channel sound (%d)", scty));
                else if (scty == 63)
                    r.msgs.emplace_back(2, strprintf("AAC sound (%d)", scty));
                else
                    r.msgs.emplace_back(2, strprintf("Unknown ASCTy (%d)", scty));

                r.msgs.emplace_back(2, strprintf("SubChannel ID=%02X", subchid));
                r.msgs.emplace_back(2, strprintf("CA=%d", ca));
            }
            else if (timd == 1) {
                // MSC stream data
                r.msgs.emplace_back(2, "Stream data mode");
                r.msgs.emplace_back(2, psdesc);
                r.msgs.emplace_back(2, strprintf("DSCTy=%d %s", scty, get_dscty_type(scty)));
                r.msgs.emplace_back(2, strprintf("SubChannel ID=%02X", subchid));
                r.msgs.emplace_back(2, strprintf("CA=%d", ca));
            }
            else if (timd == 2) {
                // FIDC
                r.msgs.emplace_back(2, "FIDC mode");
                r.msgs.emplace_back(2, psdesc);
                r.msgs.emplace_back(2, strprintf("DSCTy=%d %s", scty, get_dscty_type(scty)));
                r.msgs.emplace_back(2, strprintf("Fast Information Data Channel ID=%02X", subchid));
                r.msgs.emplace_back(2, strprintf("CA=%d", ca));
            }
            else if (timd == 3) {
                // MSC Packet mode
                r.msgs.emplace_back(2, "MSC Packet Mode");
                r.msgs.emplace_back(2, psdesc);
                r.msgs.emplace_back(2, strprintf("SubChannel ID=%02X", subchid));
                r.msgs.emplace_back(2, strprintf("CA=%d", ca));
            }

            k += 2;
        }
    }

    return r;
}

