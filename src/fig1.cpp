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

using namespace std;

// SHORT LABELS
fig_result_t fig1_select(fig1_common_t& fig1, const display_settings_t &disp)
{
    uint16_t ext,charset;
    uint16_t flag;
    char label[17];
    fig_result_t r;
    uint8_t* f = fig1.f;

    charset = (f[0] & 0xF0) >> 4;
    //oe = (f[0] & 0x08) >> 3;
    ext = f[0] & 0x07;
    r.msgs.push_back(strprintf("Charset=%d", charset));

    memcpy(label, f+fig1.figlen-18, 16);
    label[16] = 0x00;
    flag = f[fig1.figlen-2] * 256 + \
           f[fig1.figlen-1];

    switch (ext) {
        case 0: // FIG 1/0 Ensemble label
            {   // ETSI EN 300 401 8.1.13
                uint16_t eid;
                eid = f[1] * 256 + f[2];
                r.msgs.push_back(strprintf("Ensemble ID=0x%04X", eid));
                r.msgs.push_back(strprintf("Label=\"%s\"", label));
                r.msgs.push_back(strprintf("Short label mask=0x%04X", flag));

                if (fig1.fibcrccorrect) {
                    fig1.ensemble.EId = eid;
                    fig1.ensemble.label.label = label;
                    fig1.ensemble.label.shortlabel_flag = flag;
                }
            }
            break;

        case 1: // FIG 1/1 Programme service label
            {   // ETSI EN 300 401 8.1.14.1
                uint16_t sid;
                sid = f[1] * 256 + f[2];
                r.msgs.push_back(strprintf("Service ID=0x%04X", sid));
                r.msgs.push_back(strprintf("Label=\"%s\"", label));
                r.msgs.push_back(strprintf("Short label mask=0x%04X", flag));

                if (fig1.fibcrccorrect) {
                    try {
                        auto& service = fig1.ensemble.get_service(sid);
                        service.label.label = label;
                        service.label.shortlabel_flag = flag;
                    }
                    catch (ensemble_database::not_found &e) {
                        r.errors.push_back("Not yet in DB");
                    }
                }
            }
            break;

        case 4: // FIG 1/4 Service component label
            {   // ETSI EN 300 401 8.1.14.3
                uint32_t sid;
                uint8_t pd, SCIdS;
                pd    = (f[1] & 0x80) >> 7;
                SCIdS =  f[1] & 0x0F;
                if (pd == 0) {
                    sid = f[2] * 256 + \
                          f[3];
                }
                else {
                    sid = f[2] * 256 * 256 * 256 + \
                          f[3] * 256 * 256 + \
                          f[4] * 256 + \
                          f[5];
                }
                r.msgs.push_back(strprintf("Service ID=0x%04X", sid));
                r.msgs.push_back(strprintf("Service Component ID=0x%04X", SCIdS));
                r.msgs.push_back(strprintf("Label=\"%s\"", label));
                r.msgs.push_back(strprintf("Short label mask=0x%04X", flag));
            }
            break;

        case 5: // FIG 1/5 Data service label
            {   // ETSI EN 300 401 8.1.14.2
                uint32_t sid;
                sid = f[1] * 256 * 256 * 256 + \
                      f[2] * 256 * 256 + \
                      f[3] * 256 + \
                      f[4];

                r.msgs.push_back(strprintf("Service ID=0x%04X", sid));
                r.msgs.push_back(strprintf("Label=\"%s\"", label));
                r.msgs.push_back(strprintf("Short label mask=0x%04X", flag));
            }
            break;


        case 6: // FIG 1/6 X-PAD user application label
            {   // ETSI EN 300 401 8.1.14.4
                uint32_t sid;
                uint8_t pd, SCIdS, xpadapp;
                string xpadappdesc;

                pd    = (f[1] & 0x80) >> 7;
                SCIdS =  f[1] & 0x0F;
                if (pd == 0) {
                    sid = f[2] * 256 + \
                          f[3];
                    xpadapp = f[4] & 0x1F;
                }
                else {
                    sid = f[2] * 256 * 256 * 256 + \
                          f[3] * 256 * 256 + \
                          f[4] * 256 + \
                          f[5];
                    xpadapp = f[6] & 0x1F;
                }

                if (xpadapp == 2) {
                    xpadappdesc = "DLS";
                }
                else if (xpadapp == 12) {
                    xpadappdesc = "MOT";
                }
                else {
                    xpadappdesc = "?";
                }


                r.msgs.push_back(strprintf("Service ID=0x%04X", sid));
                r.msgs.push_back(strprintf("Service Component ID=0x%04X", SCIdS));
                r.msgs.push_back(strprintf("X-PAD App=%02X (", xpadapp) + xpadappdesc + ")");
                r.msgs.push_back(strprintf("Label=\"%s\"", label));
                r.msgs.push_back(strprintf("Short label mask=0x%04X", flag));
            }
            break;
    }

    // FIG1s always contain a complete set of information
    r.complete = true;
    return r;
}

