/*
    Copyright (C) 2019 Matthias P. Braendli (http://www.opendigitalradio.org)
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
#include <string>
#include <cstring>

using namespace std;

static const size_t header_length = 1; // FIG data field header

static void handle_ext_label_data_field(fig2_common_t& fig2, ensemble_database::label_t& label, const display_settings_t &disp, fig_result_t& r)
{
    const uint8_t *f = fig2.f + header_length + fig2.identifier_len();
    const size_t len_bytes = fig2.figlen - header_length - fig2.identifier_len();

    if (label.toggle_flag != fig2.toggle_flag()) {
        label.segments.clear();
        label.extended_label_charset = ensemble_database::charset_e::UNDEFINED;
        label.toggle_flag = fig2.toggle_flag();
    }

    size_t len_character_field = len_bytes;

    if (fig2.segment_index() == 0) {
        // Only if it's the first segment
        const uint8_t encoding_flag = (f[0] & 0x80) >> 7;
        const uint8_t segment_count = (f[0] & 0x70) >> 4;
        label.segment_count = segment_count + 1;

        r.msgs.push_back(strprintf("encoding=%s", (encoding_flag ? "UCS-2" : "UTF-8")));
        r.msgs.push_back(strprintf("Total number of segments=%d", segment_count + 1));

        if (encoding_flag) {
            label.extended_label_charset = ensemble_database::charset_e::UCS2;
        }
        else {
            label.extended_label_charset = ensemble_database::charset_e::UTF8;
        }

        if (fig2.rfu() == 0) {
            const uint8_t rfa = (f[0] & 0x0F);
            r.msgs.push_back(strprintf("rfa=%d", rfa));
            const uint16_t char_flag = read_u16_from_buf(f + 1);
            r.msgs.push_back(strprintf("character flag=%04x", char_flag));

            if (len_bytes <= 3) {
                throw runtime_error("FIG2 label length too short");
            }

            f += 3;
            len_character_field -= 3;
        }
        else {
            // ETSI TS 103 176 draft V2.2.1 (2018-08) gives a new meaning to rfu
            const uint8_t text_control = (f[0] & 0x0F);
            r.msgs.push_back(strprintf("text control=0x%02x", text_control));

            if (len_bytes <= 1) {
                throw runtime_error("FIG2 label length too short");
            }

            f += 1;
            len_character_field -= 1;
        }
    }

    vector<uint8_t> labelbytes(f, f + len_character_field);
    label.segments[fig2.segment_index()] = labelbytes;
}

// UTF-8 or UCS2 Labels
fig_result_t fig2_select(fig2_common_t& fig2, const display_settings_t &disp)
{
    fig_result_t r;
    const uint8_t *f = fig2.f;

    // FIG data field
    r.msgs.push_back(strprintf("toggle flag=%d", fig2.toggle_flag()));
    r.msgs.push_back(strprintf("segment index=%d", fig2.segment_index()));
    r.msgs.push_back(strprintf("rfu=%d", fig2.rfu()));

    // ext is followed by Identifier field of Type 2 field,
    // whose length depends on ext

    if (not fig2.fibcrccorrect) {
        // TODO
    }

    switch (fig2.ext()) {
        case 0: // Ensemble label
            {   // ETSI EN 300 401 8.1.13
                uint16_t eid = read_u16_from_buf(f + 1);
                if (fig2.figlen <= header_length + fig2.identifier_len()) {
                    r.errors.push_back("FIG2 length error");
                }
                else {
                    r.msgs.push_back(strprintf("Ensemble ID=0x%04X", eid));
                    handle_ext_label_data_field(fig2, fig2.ensemble.label, disp, r);

                    const auto complete_label = fig2.ensemble.label.assemble();
                    r.msgs.push_back(strprintf("Label segments=\"%s\"", fig2.ensemble.label.assembly_state().c_str()));
                    if (not complete_label.empty()) {
                        r.msgs.push_back(strprintf("Label=\"%s\"", complete_label.c_str()));
                    }
                }
            }
            break;

        case 1: // Programme service label
            {   // ETSI EN 300 401 8.1.14.1
                uint16_t sid = read_u16_from_buf(f + 1);
                if (fig2.figlen <= header_length + fig2.identifier_len()) {
                    r.errors.push_back("FIG2 length error");
                }
                else {
                    r.msgs.push_back(strprintf("Service ID=0x%04X", sid));
                    try {
                        auto& service = fig2.ensemble.get_service(sid);
                        handle_ext_label_data_field(fig2, service.label, disp, r);

                        const auto complete_label = service.label.assemble();
                        r.msgs.push_back(strprintf("Label segments=\"%s\"", service.label.assembly_state().c_str()));
                        if (not complete_label.empty()) {
                            r.msgs.push_back(strprintf("Label=\"%s\"", complete_label.c_str()));
                        }
                    }
                    catch (ensemble_database::not_found &e) {
                        r.errors.push_back("Not yet in DB");
                    }
                }
            }
            break;

        case 4: // Service component label
            {   // ETSI EN 300 401 8.1.14.3
                uint32_t sid;
                uint8_t pd    = (f[1] & 0x80) >> 7;
                uint8_t SCIdS =  f[1] & 0x0F;
                if (pd == 0) {
                    sid = read_u16_from_buf(f + 2);
                }
                else {
                    sid = read_u32_from_buf(f + 2);
                }
                if (fig2.figlen <= header_length + fig2.identifier_len()) {
                    r.errors.push_back("FIG2 length error");
                }
                else {
                    if (pd == 0) {
                        r.msgs.push_back(strprintf("Service ID=0x%04X", sid));
                    }
                    else {
                        r.msgs.push_back(strprintf("Service ID=0x%08X", sid));
                    }
                    r.msgs.push_back(strprintf("Service Component ID=0x%04X", SCIdS));

                    try {
                        auto& service = fig2.ensemble.get_service(sid);
                        auto& comp = service.get_component_by_scids(SCIdS);

                        handle_ext_label_data_field(fig2, comp.label, disp, r);

                        const auto complete_label = comp.label.assemble();
                        r.msgs.push_back(strprintf("Label segments=\"%s\"", comp.label.assembly_state().c_str()));
                        if (not complete_label.empty()) {
                            r.msgs.push_back(strprintf("Label=\"%s\"", complete_label.c_str()));
                        }
                    }
                    catch (ensemble_database::not_found &e) {
                        r.errors.push_back("Not yet in DB");
                    }
                }
            }
            break;

        case 5: // Data service label
            {   // ETSI EN 300 401 8.1.14.2
                uint32_t sid = read_u32_from_buf(f + 1);

                if (fig2.figlen <= header_length + fig2.identifier_len()) {
                    r.errors.push_back("FIG2 length error");
                }
                else {
                    r.msgs.push_back(strprintf("Service ID=0x%04X", sid));

                    try {
                        auto& service = fig2.ensemble.get_service(sid);
                        handle_ext_label_data_field(fig2, service.label, disp, r);

                        const auto complete_label = service.label.assemble();
                        r.msgs.push_back(strprintf("Label segments=\"%s\"", service.label.assembly_state().c_str()));
                        if (not complete_label.empty()) {
                            r.msgs.push_back(strprintf("Label=\"%s\"", complete_label.c_str()));
                        }
                    }
                    catch (ensemble_database::not_found &e) {
                        r.errors.push_back("Not yet in DB");
                    }
                }
            }
            break;


        case 6: // X-PAD user application label
            {   // ETSI EN 300 401 8.1.14.4
                uint32_t sid;
                uint8_t pd, SCIdS, xpadapp;
                string xpadappdesc;

                pd    = (f[1] & 0x80) >> 7;
                SCIdS =  f[1] & 0x0F;
                if (pd == 0) {
                    sid = read_u16_from_buf(f + 2);
                    xpadapp = f[4] & 0x1F;
                }
                else {
                    sid = read_u32_from_buf(f + 2);
                    xpadapp = f[6] & 0x1F;
                }

                if (fig2.figlen <= header_length + fig2.identifier_len()) {
                    r.errors.push_back("FIG2 length error");
                }
                else {
                    if (xpadapp == 2) {
                        xpadappdesc = "DLS";
                    }
                    else if (xpadapp == 12) {
                        xpadappdesc = "MOT";
                    }
                    else {
                        xpadappdesc = "?";
                    }

                    ensemble_database::label_t label;
                    // TODO handle multi-segment labels

                    handle_ext_label_data_field(fig2, label, disp, r);

                    const auto complete_label = label.assemble();
                    r.msgs.push_back(strprintf("Label segments=\"%s\"", label.assembly_state().c_str()));
                    if (not complete_label.empty()) {
                        r.msgs.push_back(strprintf("Label=\"%s\"", complete_label.c_str()));
                    }

                    r.msgs.push_back(strprintf("Service ID=0x%04X", sid));
                    r.msgs.push_back(strprintf("Service Component ID=0x%04X", SCIdS));
                    r.msgs.push_back(strprintf("X-PAD App=%02X (", xpadapp) + xpadappdesc + ")");
                }
            }
            break;
    }

    // FIG2s always contain a complete set of information
    r.complete = true;
    return r;
}

