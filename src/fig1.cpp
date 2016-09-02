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
#include <string>
#include <cstring>

using namespace std;

// SHORT LABELS
bool fig1_select(fig1_common_t& fig1, int indent)
{
    uint16_t ext,oe,charset;
    uint16_t flag;
    char label[17];
    char desc[256];
    uint8_t* f = fig1.f;
    const int figtype = 1;

    charset = (f[0] & 0xF0) >> 4;
    oe = (f[0] & 0x08) >> 3;
    ext = f[0] & 0x07;
    sprintf(desc,
            "FIG %d/%d: OE=%d, Charset=%d",
            figtype, ext, oe, charset);

    printbuf(desc, indent, f+1, fig1.figlen-1);
    memcpy(label, f+fig1.figlen-18, 16);
    label[16] = 0x00;
    flag = f[fig1.figlen-2] * 256 + \
           f[fig1.figlen-1];

    switch (ext) {
        case 0: // FIG 1/0 Ensemble label
            {   // ETSI EN 300 401 8.1.13
                uint16_t eid;
                eid = f[1] * 256 + f[2];
                sprintf(desc, "Ensemble ID 0x%04X label: \"%s\", Short label mask: 0x%04X", eid, label, flag);
                printinfo(desc, indent+1, 1);
            }
            break;

        case 1: // FIG 1/1 Programme service label
            {   // ETSI EN 300 401 8.1.14.1
                uint16_t sid;
                sid = f[1] * 256 + f[2];
                sprintf(desc, "Service ID 0x%X label: \"%s\", Short label mask: 0x%04X", sid, label, flag);
                printinfo(desc, indent+1, 1);
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
                sprintf(desc,
                        "Service ID  0x%X , Service Component ID 0x%04X Short, label: \"%s\", label mask: 0x%04X",
                        sid, SCIdS, label, flag);
                printinfo(desc, indent+1, 1);
            }
            break;

        case 5: // FIG 1/5 Data service label
            {   // ETSI EN 300 401 8.1.14.2
                uint32_t sid;
                sid = f[1] * 256 * 256 * 256 + \
                      f[2] * 256 * 256 + \
                      f[3] * 256 + \
                      f[4];

                sprintf(desc,
                        "Service ID 0x%X label: \"%s\", Short label mask: 0x%04X",
                        sid, label, flag);
                printinfo(desc, indent+1, 1);
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


                sprintf(desc,"Service ID  0x%X , Service Component ID 0x%04X Short, X-PAD App %02X (%s), label: \"%s\", label mask: 0x%04X",
                        sid, SCIdS, xpadapp, xpadappdesc.c_str(), label, flag);
                printbuf(desc,indent+1,NULL,0,"");
            }
            break;
    }

    // FIG1s always contain a complete set of information
    return true;
}

