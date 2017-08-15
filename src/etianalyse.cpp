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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <algorithm>
#include <cassert>
#include "etianalyse.hpp"
#include "etiinput.hpp"
#include "figs.hpp"

extern "C" {
#include "lib_crc.h"
}

#include "utils.hpp"

using namespace std;

// Signal handler flag
std::atomic<bool> quit(false);

bool
eti_analyse_config_t::is_fig_to_be_printed(int type, int extension) const
{
    if (figs_to_display.empty()) {
        return true;
    }

    return std::find(
            figs_to_display.begin(),
            figs_to_display.end(),
            make_pair(type, extension)) != figs_to_display.end();
}

static void print_fig_result(const fig_result_t& fig_result, const display_settings_t& disp)
{
    if (disp.print) {
        for (const auto& msg : fig_result.msgs) {
            std::string s;
            for (int i = 0; i < msg.level; i++) {
                s += "    ";
            }
            s += msg.msg;
            for (int i = 0; i < disp.indent; i++) {
                printf("\t");
            }
            printf("%s\n", s.c_str());
        }
        if (not fig_result.errors.empty()) {
            printf("ERRORS:\n");
            for (const auto& err : fig_result.errors) {
                for (int i = 0; i < disp.indent; i++) {
                    printf("\t");
                }
                printf("%s\n", err.c_str());
            }
        }
    }
}


void ETI_Analyser::eti_analyse()
{
    uint8_t p[ETINIPACKETSIZE];
    string desc;
    char prevsync[3]={0x00,0x00,0x00};
    uint8_t ficf,nst,fp,mid,ficl;
    uint16_t fl,crch;
    uint16_t crc;
    uint8_t scid,tpl;
    uint16_t sad[64],stl[64];
    char sdesc[256];
    uint32_t frame_nb = 0, frame_sec = 0, frame_ms = 0, frame_h, frame_m, frame_s;

    static int last_fct = -1;

    bool running = true;
    size_t num_frames = 0;

    int stream_type = ETI_STREAM_TYPE_NONE;
    if (identify_eti_format(config.etifd, &stream_type) == -1) {
        printf("Could not identify stream type\n");

        running = false;
    }
    else {
        printf("Identified ETI type ");
        if (stream_type == ETI_STREAM_TYPE_RAW)
            printf("RAW\n");
        else if (stream_type == ETI_STREAM_TYPE_STREAMED)
            printf("STREAMED\n");
        else if (stream_type == ETI_STREAM_TYPE_FRAMED)
            printf("FRAMED\n");
        else
            printf("?\n");
    }

    if (config.analyse_fig_rates) {
        rate_display_header(config.analyse_fig_rates_per_second);
    }

    FILE *stat_fd = nullptr;
    if (not config.statistics_filename.empty()) {
        stat_fd = fopen(config.statistics_filename.c_str(), "w");
        if (stat_fd == nullptr) {
            fprintf(stderr, "Could not open statistics file: %s\n",
                    strerror(errno));
            return;
        }
    }

    while (running) {

        int ret = get_eti_frame(config.etifd, stream_type, p);
        if (ret == -1) {
            fprintf(stderr, "ETI file read error\n");
            break;
        }
        else if (ret == 0) {
            fprintf(stderr, "End of ETI\n");
            break;
        }

        // Timestamp and Frame Number
        frame_h = (frame_sec / 3600);
        frame_m = (frame_sec - (frame_h * 3600)) / 60;
        frame_s = (frame_sec - (frame_h * 3600) - (frame_m * 60));
        sprintf(sdesc, "%02d:%02d:%02d.%03d frame %d", frame_h, frame_m, frame_s, frame_ms, frame_nb);
        printbuf(sdesc, 0, NULL, 0);
        frame_ms += 24; // + 24 ms
        if (frame_ms >= 1000) {
            frame_ms -= 1000;
            frame_sec++;
        }
        frame_nb++;

        // SYNC
        printbuf("SYNC", 0, p, 4);

        // SYNC - ERR
        if (p[0] == 0xFF) {
            desc = "No error";
            printbuf("ERR", 1, p, 1, desc);
        }
        else {
            desc = "Error";
            printbuf("ERR", 1, p, 1, desc);
            if (!config.ignore_error) {
                printf("Aborting because of SYNC error\n");
                break;
            }
        }

        // SYNC - FSYNC

        if (memcmp(prevsync, "\x00\x00\x00", 3) == 0) {
            if ( (memcmp(p + 1, "\x07\x3a\xb6", 3) == 0) ||
                 (memcmp(p + 1, "\xf8\xc5\x49", 3) == 0) ) {
                desc = "OK";
                memcpy(prevsync, p+1, 3);
            }
            else {
                desc ="Wrong FSYNC";
                memcpy(prevsync, "\x00\x00\x00", 3);
            }
        } else if (memcmp(prevsync, "\x07\x3a\xb6", 3) == 0) {
            if (memcmp(p + 1, "\xf8\xc5\x49", 3) != 0) {
                desc = "Wrong FSYNC";
                memcpy(prevsync, "\x00\x00\x00", 3);
            } else {
                desc = "OK";
                memcpy(prevsync, p + 1, 3);
            }
        } else if (memcmp(prevsync, "\xf8\xc5\x49", 3) == 0) {
            if (memcmp(p + 1, "\x07\x3a\xb6", 3) != 0) {
                desc = "Wrong FSYNC";
                memcpy(prevsync, "\x00\x00\x00", 3);
            } else {
                desc = "OK";
                memcpy(prevsync, p + 1, 3);
            }
        }
        printbuf("Sync FSYNC", 1, p + 1, 3, desc);

        // LIDATA
        printbuf("LDATA", 0, NULL, 0);
        // LIDATA - FC
        printbuf("FC - Frame Characterization field", 1, p+4, 4);
        // LIDATA - FC - FCT
        char fct_str[25];
        sprintf(fct_str, "%d", p[4]);
        int fct = p[4];
        printbuf("FCT  - Frame Count", 2, p+4, 1, fct_str);
        if (last_fct != -1) {
            if ((last_fct + 1) % 250 != fct) {
                printf("FCT not contiguous\n");
            }
        }
        last_fct = fct;
        // LIDATA - FC - FICF
        ficf = (p[5] & 0x80) >> 7;

        {
            stringstream ss;
            ss << (int)ficf;
            if (ficf == 1) {
                ss << "- FIC Information are present";
            }
            else {
                ss << "- FIC Information are not present";
            }

            printbuf("FICF - Fast Information Channel Flag", 2, NULL, 0, ss.str());
        }

        // LIDATA - FC - NST
        nst = p[5] & 0x7F;
        {
            stringstream ss;
            ss << (int)nst;
            printbuf("NST  - Number of streams", 2, NULL, 0, ss.str());
        }

        // LIDATA - FC - FP
        fp = (p[6] & 0xE0) >> 5;
        {
            stringstream ss;
            ss << (int)fp;
            printbuf("FP   - Frame Phase", 2, &fp, 1, ss.str());
        }

        // LIDATA - FC - MID
        mid = (p[6] & 0x18) >> 3;
        {
            stringstream ss;
            ss << "Mode ";
            if (mid != 0) {
                ss << (int)mid;
            }
            else {
                ss << "4";
            }
            printbuf("MID  - Mode Identity", 2, &mid, 1, ss.str());
            set_mode_identity(mid);
        }

        // LIDATA - FC - FL
        fl = (p[6] & 0x07) * 256 + p[7];
        {
            stringstream ss;
            ss << fl << " words";
            printbuf("FL   - Frame Length", 2, NULL, 0, ss.str());
        }

        if (ficf == 0) {
            ficl = 0;
        }
        else if (mid == 3) {
            ficl = 32;
        }
        else {
            ficl = 24;
        }

        // STC
        printbuf("STC - Stream Characterisation", 1, NULL, 0);

        for (int i=0; i < nst; i++) {
            sprintf(sdesc, "Stream number %d", i);
            printbuf("STC  - Stream Characterisation", 2, p + 8 + 4*i, 4, sdesc);
            scid = (p[8 + 4*i] & 0xFC) >> 2;
            sprintf(sdesc, "%d", scid);
            printbuf("SCID - Sub-channel Identifier", 3, NULL, 0, sdesc);
            sad[i] = (p[8+4*i] & 0x03) * 256 + p[9+4*i];
            sprintf(sdesc, "%d", sad[i]);
            printbuf("SAD  - Sub-channel Start Address", 3, NULL, 0, sdesc);
            tpl = (p[10+4*i] & 0xFC) >> 2;

            if ((tpl & 0x20) >> 5 == 1) {
                uint8_t opt, plevel;
                string plevelstr;
                opt = (tpl & 0x1c) >> 2;
                plevel = (tpl & 0x03);
                if (opt == 0x00) {
                    if (plevel == 0)
                        plevelstr = "1-A, 1/4, 16 CUs";
                    else if (plevel == 1)
                        plevelstr = "2-A, 3/8, 8 CUs";
                    else if (plevel == 2)
                        plevelstr = "3-A, 1/2, 6 CUs";
                    else if (plevel == 3)
                        plevelstr = "4-A, 3/4, 4 CUs";
                }
                else if (opt == 0x01) {
                    if (plevel == 0)
                        plevelstr = "1-B, 4/9, 27 CUs";
                    else if (plevel == 1)
                        plevelstr = "2-B, 4/7, 21 CUs";
                    else if (plevel == 2)
                        plevelstr = "3-B, 4/6, 18 CUs";
                    else if (plevel == 3)
                        plevelstr = "4-B, 4/5, 15 CUs";
                }
                else {
                    stringstream ss;
                    ss << "Unknown option " << opt;
                    plevelstr = ss.str();
                }
                sprintf(sdesc, "0x%02x - Equal Error Protection. %s", tpl, plevelstr.c_str());
            }
            else {
                uint8_t tsw, uepidx;
                tsw = (tpl & 0x08);
                uepidx = tpl & 0x07;
                sprintf(sdesc, "0x%02x - Unequal Error Protection. Table switch %d,  UEP index %d", tpl, tsw, uepidx);
            }
            printbuf("TPL  - Sub-channel Type and Protection Level", 3, NULL, 0, sdesc);
            stl[i] = (p[10+4*i] & 0x03) * 256 + \
                      p[11+4*i];
            sprintf(sdesc, "%d => %d kbit/s", stl[i], stl[i]*8/3);
            printbuf("STL  - Sub-channel Stream Length", 3, NULL, 0, sdesc);

            if (config.statistics and config.streams_to_decode.count(i) == 0) {
                config.streams_to_decode.emplace(std::piecewise_construct,
                        std::make_tuple(i),
                        std::make_tuple(false)); // do not dump to file
                config.streams_to_decode.at(i).subchid = scid;
            }

            if (config.streams_to_decode.count(i) > 0) {
                config.streams_to_decode.at(i).set_subchannel_index(stl[i]/3);
                config.streams_to_decode.at(i).set_index(i);
                config.streams_to_decode.at(i).subchid = scid;
            }
        }

        // EOH
        printbuf("EOH - End Of Header", 1, p + 8 + 4*nst, 4);
        uint16_t mnsc = p[8 + 4*nst] * 256 + \
                                  p[8 + 4*nst + 1];
        {
            stringstream ss;
            ss << mnsc;
            printbuf("MNSC - Multiplex Network Signalling Channel", 2, p+8+4*nst, 2, ss.str());
        }

        crch = p[8 + 4*nst + 2]*256 + \
               p[8 + 4*nst + 3];
        crc  = 0xffff;

        for (int i=4; i < 8 + 4*nst + 2; i++)
            crc = update_crc_ccitt(crc, p[i]);
        crc =~ crc;

        if (crc == crch) {
            sprintf(sdesc,"CRC OK");
        }
        else {
            sprintf(sdesc,"CRC Mismatch: %02x",crc);
        }

        printbuf("Header CRC", 2, p + 8 + 4*nst + 2, 2, sdesc);

        // MST - FIC
        if (ficf == 1) {
            uint8_t *fib, *fig;

            FIGalyser figs;

            uint8_t ficdata[32*4];
            memcpy(ficdata, p + 12 + 4*nst, ficl*4);
            sprintf(sdesc, "FIC Data (%d bytes)", ficl*4);
            //printbuf(sdesc, 1, ficdata, ficl*4);
            printbuf(sdesc, 1, NULL, 0);
            fib = p + 12 + 4*nst;
            for (int i = 0; i < ficl*4/32; i++) {
                sprintf(sdesc, "FIB %d", i);
                printbuf(sdesc, 1, NULL, 0);
                fig=fib;
                figs.set_fib(i);
                rate_new_fib(i);

                const uint16_t figcrc = fib[30]*256 + fib[31];
                crc = 0xffff;
                for (int j = 0; j < 30; j++) {
                    crc = update_crc_ccitt(crc, fib[j]);
                }
                crc =~ crc;
                const bool crccorrect = (crc == figcrc);
                if (crccorrect)
                    sprintf(sdesc, "FIB %d CRC OK", i);
                else
                    sprintf(sdesc, "FIB %d CRC Mismatch: %02x", i, crc);


                bool endmarker = false;
                int figcount = 0;
                while (!endmarker) {
                    uint8_t figtype, figlen;
                    figtype = (fig[0] & 0xE0) >> 5;
                    if (figtype != 7) {
                        figlen = fig[0] & 0x1F;
                        sprintf(sdesc, "FIG %d [%d bytes]", figtype, figlen);
                        printbuf(sdesc, 3, fig+1, figlen);
                        decodeFIG(config, figs, fig+1, figlen, figtype, 4, crccorrect);
                        fig += figlen + 1;
                        figcount += figlen + 1;
                        if (figcount >= 29)
                            endmarker = true;
                    }
                    else {
                        endmarker = true;
                    }
                }

                printbuf(sdesc,3,fib+30,2);

                fib += 32;
            }

            if (config.analyse_fic_carousel) {
                figs.analyse();
            }
        }

        int offset = 0;
        for (int i=0; i < nst; i++) {
            uint8_t streamdata[684*8];
            memcpy(streamdata, p + 12 + 4*nst + ficf*ficl*4 + offset, stl[i]*8);
            offset += stl[i] * 8;
            if (config.streams_to_decode.count(i) > 0) {
                sprintf(sdesc, "id %d, len %d, selected for decoding", i, stl[i]*8);
            }
            else {
                sprintf(sdesc, "id %d, len %d, not selected for decoding", i, stl[i]*8);
            }
            if (get_verbosity() > 2) {
                printbuf("Stream Data", 1, streamdata, stl[i]*8, sdesc);
            }
            else {
                printbuf("Stream Data", 1, streamdata, 0, sdesc);
            }

            if (config.streams_to_decode.count(i) > 0) {
                config.streams_to_decode.at(i).push(streamdata, stl[i]*8);
            }

        }

        //* EOF (4 Bytes)
        // CRC (2 Bytes)
        crch = p[12 + 4*nst + ficf*ficl*4 + offset] * 256 + \
               p[12 + 4*nst + ficf*ficl*4 + offset + 1];

        crc = 0xffff;

        for (int i = 12 + 4*nst; i < 12 + 4*nst + ficf*ficl*4 + offset; i++)
            crc = update_crc_ccitt(crc, p[i]);
        crc =~ crc;
        if (crc == crch)
            sprintf(sdesc, "CRC OK");
        else
            sprintf(sdesc, "CRC Mismatch: %02x", crc);

        printbuf("EOF", 1, p + 12 + 4*nst + ficf*ficl*4 + offset, 4);
        printbuf("CRC", 2, p + 12 + 4*nst + ficf*ficl*4 + offset, 2, sdesc);

        // RFU (2 Bytes)
        printbuf("RFU", 2, p + 12 + 4*nst + ficf*ficl*4 + offset + 2, 2);

        //* TIST (4 Bytes)
        const size_t tist_ix = 12 + 4*nst + ficf*ficl*4 + offset + 4;
        uint32_t TIST = (uint32_t)(p[tist_ix]) << 24 |
                        (uint32_t)(p[tist_ix+1]) << 16 |
                        (uint32_t)(p[tist_ix+2]) << 8 |
                        (uint32_t)(p[tist_ix+3]);

        sprintf(sdesc, "%f ms", (TIST & 0xFFFFFF) / 16384.0);
        printbuf("TIST - Time Stamp", 1, p + tist_ix, 4, sdesc);

        if (get_verbosity()) {
            printf("-------------------------------------------------------------------------------------------------------------\n");
        }

        if (config.analyse_fig_rates and (fct % 250) == 0) {
            rate_display_analysis(false, config.analyse_fig_rates_per_second);
        }

        num_frames++;
        if (config.num_frames_to_decode > 0 and
                num_frames >= config.num_frames_to_decode) {
            printf("Decoded %zu ETI frames\n", num_frames);
            break;
        }

        if (quit.load()) running = false;
    }

    if (config.statistics) {
        assert(stat_fd != nullptr);

        fprintf(stat_fd, "# Statistics from ETISnoop. This file should be valid YAML\n");
        fprintf(stat_fd, "---\n");
        fprintf(stat_fd, "ensemble:\n");
        fprintf(stat_fd, "    id: 0x%x\n", ensemble.EId);
        fprintf(stat_fd, "    label: %s\n", ensemble.label.c_str());
        fprintf(stat_fd, "    shortlabel: %s\n",
                flag_to_shortlabel(ensemble.label, ensemble.shortlabel_flag).c_str());
        fprintf(stat_fd, "audio:\n");

        for (const auto& snoop : config.streams_to_decode) {
            bool corresponding_service_found = false;

            for (const auto& service : ensemble.services) {
                for (const auto& component : service.components) {
                    if (component.subchId == snoop.second.subchid and
                            component.primary) {
                        corresponding_service_found = true;
                        fprintf(stat_fd, "    - service_id: 0x%x\n", service.id);
                        fprintf(stat_fd, "      subchannel_id: 0x%x\n", component.subchId);
                        fprintf(stat_fd, "      label: %s\n", service.label.c_str());
                        fprintf(stat_fd, "      shortlabel: %s\n",
                                flag_to_shortlabel(service.label, service.shortlabel_flag).c_str());
                        if (not component.label.empty()) {
                            fprintf(stat_fd, "      component_label: %s\n", component.label.c_str());
                        }

                        try {
                            const auto& subch = ensemble.get_subchannel(component.subchId);
                            fprintf(stat_fd, "      subchannel:\n");
                            fprintf(stat_fd, "          id: %d\n", subch.id);
                            fprintf(stat_fd, "          SAd: %d\n", subch.start_addr);
                            switch (subch.protection_type) {
                                case ensemble_database::subchannel_t::protection_type_t::EEP:
                                    if (subch.protection_option == 0) {
                                        fprintf(stat_fd, "          protection: EEP %d-A\n",
                                                subch.protection_level + 1);
                                    }
                                    else if (subch.protection_option == 0) {
                                        fprintf(stat_fd, "          protection: EEP %d-B\n",
                                                subch.protection_level + 1);
                                    }
                                    else {
                                        fprintf(stat_fd, "          protection: unknown\n");
                                    }

                                    fprintf(stat_fd, "          size: %d\n", subch.size);
                                    break;
                                case ensemble_database::subchannel_t::protection_type_t::UEP:
                                    fprintf(stat_fd, "          table_switch: %d\n", subch.table_switch);
                                    fprintf(stat_fd, "          table_index: %d\n", subch.table_index);
                                    break;
                            }
                        }
                        catch (ensemble_database::not_found &e)
                        {
                            fprintf(stat_fd, "      subchannel: not found\n");
                        }
                    }
                }
            }

            if (not corresponding_service_found) {
                fprintf(stat_fd, "    - service_id: unknown\n");
            }

            const auto& stat = snoop.second.get_audio_statistics();
            fprintf(stat_fd, "      audio:\n");
            fprintf(stat_fd, "          average: %d %d\n",
                    absolute_to_dB(stat.average_level_left),
                    absolute_to_dB(stat.average_level_right));
            fprintf(stat_fd, "          peak: %d %d\n",
                    absolute_to_dB(stat.peak_level_left),
                    absolute_to_dB(stat.peak_level_right));
        }

        fclose(stat_fd);
    }


    if (config.decode_watermark) {
        std::string watermark(wm_decoder.calculate_watermark());
        printf("Watermark:\n  %s\n", watermark.c_str());
    }

    if (config.analyse_fig_rates) {
        rate_display_analysis(false, config.analyse_fig_rates_per_second);
    }

    figs_cleardb();
}

void ETI_Analyser::decodeFIG(
        const eti_analyse_config_t &config,
        FIGalyser &figs,
        uint8_t* f,
        uint8_t figlen,
        uint16_t figtype,
        int indent,
        bool fibcrccorrect)
{
    char desc[512];

    switch (figtype) {
        case 0:
            {
                fig0_common_t fig0(f, figlen, ensemble, wm_decoder);
                fig0.fibcrccorrect = fibcrccorrect;

                const display_settings_t disp(config.is_fig_to_be_printed(figtype, fig0.ext()), indent);

                if (disp.print) {
                    sprintf(desc, "FIG %d/%d: C/N=%d OE=%d P/D=%d",
                            figtype, fig0.ext(), fig0.cn(), fig0.oe(), fig0.pd());
                    printfig(desc, disp, f+1, figlen-1);
                }

                figs.push_back(figtype, fig0.ext(), figlen);

                auto fig_result = fig0_select(fig0, disp);
                fig_result.figtype = figtype;
                fig_result.figext = fig0.ext();
                print_fig_result(fig_result, disp+1);

                rate_announce_fig(figtype, fig0.ext(), fig_result.complete);
            }
            break;

        case 1:
            {// SHORT LABELS
                fig1_common_t fig1(ensemble, f, figlen);
                fig1.fibcrccorrect = fibcrccorrect;

                const display_settings_t disp(config.is_fig_to_be_printed(figtype, fig1.ext()), indent);
                if (disp.print) {
                    sprintf(desc, "FIG %d/%d: OE=%d",
                            figtype, fig1.ext(), fig1.oe());
                    printfig(desc, disp, f+1, figlen-1);
                }

                figs.push_back(figtype, fig1.ext(), figlen);

                auto fig_result = fig1_select(fig1, disp);
                fig_result.figtype = figtype;
                fig_result.figext = fig1.ext();
                print_fig_result(fig_result, disp+1);
                rate_announce_fig(figtype, fig1.ext(), fig_result.complete);
            }
            break;
        case 2:
            {// EXTENDED LABELS
                uint16_t ext,oe;

                uint8_t toggle_flag = (f[0] & 0x80) >> 7;
                uint8_t segment_index = (f[0] & 0x70) >> 4;
                oe = (f[0] & 0x08) >> 3;
                ext = f[0] & 0x07;

                const display_settings_t disp(config.is_fig_to_be_printed(figtype, ext), indent);
                if (disp.print) {
                    sprintf(desc,
                            "FIG %d/%d: Toggle flag=%d, Segment_index=%d, OE=%d",
                            figtype, ext, toggle_flag, segment_index, oe);

                    printfig(desc, disp, f+1, figlen-1);
                }

                figs.push_back(figtype, ext, figlen);

                bool complete = true;
                rate_announce_fig(figtype, ext, complete);
            }
            break;
        case 5:
            {// FIDC
                uint16_t ext;

                uint8_t d1 = (f[0] & 0x80) >> 7;
                uint8_t d2 = (f[0] & 0x40) >> 6;
                uint8_t tcid = (f[0] & 0x38) >> 5;
                ext = f[0] & 0x07;

                const display_settings_t disp(config.is_fig_to_be_printed(figtype, ext), indent);

                sprintf(desc,
                        "FIG %d/%d: D1=%d, D2=%d, TCId=%d",
                        figtype, ext, d1, d2, tcid);
                printfig(desc, disp, f+1, figlen-1);

                figs.push_back(figtype, ext, figlen);

                bool complete = true; // TODO verify
                rate_announce_fig(figtype, ext, complete);
            }
            break;
        case 6:
            {// Conditional access
                fprintf(stderr, "ERROR: ETI contains unsupported FIG 6\n");
            }
            break;
        default:
            {
                fprintf(stderr, "ERROR: ETI contains unknown FIG %d\n", figtype);
            }
            break;
    }
}
