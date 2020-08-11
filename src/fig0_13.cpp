/*
    Copyright (C) 2014 CSP Innovazione nelle ICT s.c.a r.l. (http://www.csp.it/)
    Copyright (C) 2020 Matthias P. Braendli (http://www.opendigitalradio.org)
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
#include <set>

/* EN 300 401, 8.1.20, User application information
 * The combination of the SId and the SCIdS provides a service component
 * identifier which is valid globally.
 */
using SId_t = int;
using SCIdS_t = int;
static std::set<std::pair<SId_t, SCIdS_t> > components_ids_seen;

bool fig0_13_is_complete(SId_t SId, SCIdS_t SCIdS)
{
    auto key = std::make_pair(SId, SCIdS);
    bool complete = components_ids_seen.count(key);

    if (complete) {
        components_ids_seen.clear();
    }
    else {
        components_ids_seen.insert(key);
    }

    return complete;
}


std::string get_fig_0_13_userapp(int user_app_type)
{
    switch (user_app_type) {
        case 0x000: return "Reserved for future definition";
        case 0x001: return "Not used";
        case 0x002: return "MOT Slideshow";
        case 0x003: return "MOT Broadacst Web Site";
        case 0x004: return "TPEG";
        case 0x005: return "DGPS";
        case 0x006: return "TMC";
        case 0x007: return "EPG";
        case 0x008: return "DAB Java";
        case 0x44a: return "Journaline";
        default: return "Reserved for future applications";
    }
}

// FIG 0/13 User application information
// ETSI EN 300 401 8.1.20
fig_result_t fig0_13(fig0_common_t& fig0, const display_settings_t &disp)
{
    uint32_t SId;
    uint8_t  SCIdS;
    uint8_t  No;
    uint8_t* f = fig0.f;
    fig_result_t r;
    bool complete = false;

    int k = 1;

    if (fig0.pd() == 0) { // Programme services, 16 bit SId
        SId  = (f[k] << 8) |
                f[k+1];
        k+=2;

        SCIdS = f[k] >> 4;
        No    = f[k] & 0x0F;
        k++;
    }
    else { // Data services, 32 bit SId
        SId   = (f[k]   << 24) |
            (f[k+1] << 16) |
            (f[k+2] << 8) |
            f[k+3];
        k+=4;

        SCIdS = f[k] >> 4;
        No    = f[k] & 0x0F;
        k++;

    }

    complete |= fig0_13_is_complete(SId, SCIdS);

    r.msgs.emplace_back(strprintf("SId=0x%X", SId));
    r.msgs.emplace_back(strprintf("SCIdS=%u", SCIdS));

    r.msgs.emplace_back(strprintf("User applications(%d):", No));
    for (int numapp = 0; numapp < No; numapp++) {
        uint16_t user_app_type = ((f[k] << 8) |
                (f[k+1] & 0xE0)) >> 5;
        uint8_t  user_app_len  = f[k+1] & 0x1F;
        k += 2;

        r.msgs.emplace_back(1, "-");
        r.msgs.emplace_back(2, strprintf("User Application=%d '%s'",
                user_app_type,
                get_fig_0_13_userapp(user_app_type).c_str()));
        r.msgs.emplace_back(2, strprintf("length=%u", user_app_len));

        if (user_app_len >= 2) {
            bool ca_flag = (f[k] >> 7) & 0x1;
            r.msgs.emplace_back(2, strprintf("CAflag=%d", ca_flag));

            bool ca_org_flag = (f[k] >> 6) & 0x1;
            r.msgs.emplace_back(2, strprintf("CAOrgflag=%d", ca_org_flag));

            uint8_t xpad_appty = f[k] & 0x1F;
            r.msgs.emplace_back(2, strprintf("XPAD_AppTy=%d", xpad_appty));

            bool dg_flag = (f[k+1] >> 7) & 0x1;
            r.msgs.emplace_back(2, strprintf("DGflag=%d", dg_flag));

            uint8_t dscty = f[k+1] & 0x3F;
            r.msgs.emplace_back(2, strprintf("DSCTy=%d", dscty));

            k += 2;
            size_t effective_uadata_len = user_app_len - 2;

            if (ca_org_flag) {
                if (user_app_len < 4) {
                    r.errors.push_back("User Application Data Length too short to contain CAOrg!");
                }
                else {
                    uint16_t ca_org = (f[k] << 8) | f[k+1];
                    k += 2;
                    effective_uadata_len -= 2;
                    r.msgs.emplace_back(2, strprintf("ca_org=%u", ca_org));
                }
            }

            std::string ua_data = "UA Data=";
            for (size_t i = 0; i < effective_uadata_len; i++) {
                ua_data += strprintf("0x%02x ", f[k + i]);
            }
            r.msgs.emplace_back(2, move(ua_data));
        }


    }

    r.complete = complete;
    return r;
}

