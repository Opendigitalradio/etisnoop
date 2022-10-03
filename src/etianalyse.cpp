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

static
string replace_first(const string& source, const string& from, const string& to)
{
    string s(source);
    size_t i = s.find(from);
    if (i != string::npos) {
        s.replace(i, from.length(), to);
    }
    return s;
}

static void print_fig_result(const fig_result_t& fig_result, const display_settings_t& disp)
{
    if (disp.print) {
        for (const auto& msg : fig_result.msgs) {
            std::string s;
            for (int i = 0; i < msg.level; i++) {
                s += " ";
            }
            s += replace_first(msg.msg, "=", ": ");
            for (int i = 0; i < disp.indent; i++) {
                printf(" ");
            }
            printf("%s\n", s.c_str());
        }
        if (not fig_result.errors.empty()) {
            for (const auto& err : fig_result.errors) {
                fprintf(stderr, "ERRORS: %s\n" , err.c_str());
            }
        }
    }
}

void ETI_Analyser::analyse()
{
    if (config.etifd != nullptr) {
        return eti_analyse();
    }
    else if (config.ficfd != nullptr) {
        return fic_analyse();
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
    uint32_t frame_nb = 0, frame_sec = 0, frame_ms = 0;

    static int last_fct = -1;

    bool running = true;
    size_t num_frames = 0;

    int stream_type = ETI_STREAM_TYPE_NONE;
    if (identify_eti_format(config.etifd, &stream_type) == -1) {
        fprintf(stderr, "Could not identify stream type\n");

        running = false;
    }
    else {
        fprintf(stderr, "Identified ETI type ");
        if (stream_type == ETI_STREAM_TYPE_RAW)
            fprintf(stderr, "RAW\n");
        else if (stream_type == ETI_STREAM_TYPE_STREAMED)
            fprintf(stderr, "STREAMED\n");
        else if (stream_type == ETI_STREAM_TYPE_FRAMED)
            fprintf(stderr, "FRAMED\n");
        else
            fprintf(stderr, "?\n");
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
        uint32_t frame_h = (frame_sec / 3600);
        uint32_t frame_m = (frame_sec - (frame_h * 3600)) / 60;
        uint32_t frame_s = (frame_sec - (frame_h * 3600) - (frame_m * 60));
        printf("---\n");
        printf("Frame: %d\n", frame_nb);
        printf("Time: %02d:%02d:%02d.%03d\n", frame_h, frame_m, frame_s, frame_ms);
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
            printbuf("ERR", 1, p, 1, "", "No Error");
        }
        else {
            printbuf("ERR", 1, p, 1, "", "Error");
            if (!config.ignore_error) {
                fprintf(stderr, "Aborting because of SYNC error\n");
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
                desc = "Wrong FSYNC";
                memcpy(prevsync, "\x00\x00\x00", 3);
            }
        }
        else if (memcmp(prevsync, "\x07\x3a\xb6", 3) == 0) {
            if (memcmp(p + 1, "\xf8\xc5\x49", 3) != 0) {
                desc = "Wrong FSYNC";
                memcpy(prevsync, "\x00\x00\x00", 3);
            } else {
                desc = "OK";
                memcpy(prevsync, p + 1, 3);
            }
        }
        else if (memcmp(prevsync, "\xf8\xc5\x49", 3) == 0) {
            if (memcmp(p + 1, "\x07\x3a\xb6", 3) != 0) {
                desc = "Wrong FSYNC";
                memcpy(prevsync, "\x00\x00\x00", 3);
            } else {
                desc = "OK";
                memcpy(prevsync, p + 1, 3);
            }
        }
        printbuf("FSYNC", 1, p + 1, 3, "", desc);

        // LIDATA
        printbuf("LIDATA");
        // LIDATA - FC
        printbuf("FC", 1, p+4, 4, "Frame Characterization field");
        // LIDATA - FC - FCT
        int fct = p[4];
        printbuf("FCT", 2, p+4, 1, "Frame Count", to_string(fct));
        if (last_fct != -1) {
            if ((last_fct + 1) % 250 != fct) {
                fprintf(stderr, "Error: FCT not contiguous\n");
            }
        }
        last_fct = fct;
        // LIDATA - FC - FICF
        ficf = (p[5] & 0x80) >> 7;

        {
            stringstream ss;
            if (ficf == 1) {
                ss << "FIC Information are present";
            }
            else {
                ss << "FIC Information are not present";
            }

            printbuf("FICF", 2, nullptr, 0, ss.str(), to_string(ficf));
        }

        // LIDATA - FC - NST
        nst = p[5] & 0x7F;
        {
            printbuf("NST", 2, nullptr, 0, "Number of streams", to_string(nst));
        }

        // LIDATA - FC - FP
        fp = (p[6] & 0xE0) >> 5;
        {
            stringstream ss;
            ss << (int)fp;
            printbuf("FP", 2, &fp, 1, "Frame Phase", to_string(fp));
        }

        // LIDATA - FC - MID
        mid = (p[6] & 0x18) >> 3;
        {
            string modestr;
            if (mid != 0) {
                modestr = to_string(mid);
            }
            else {
                modestr = "4";
            }
            printbuf("MID", 2, &mid, 1, "Mode Identity", modestr);
            set_mode_identity(mid);
        }

        // LIDATA - FC - FL
        fl = (p[6] & 0x07) * 256uL + p[7];
        {
            printbuf("FL", 2, nullptr, 0, "Frame Length in words", to_string(fl));
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
        printvalue("STC", 1);

        for (int i=0; i < nst; i++) {
            printsequencestart(2);
            printbuf("Stream Number", 3, p + 8 + 4*i, 4, "", to_string(i));
            scid = (p[8 + 4*i] & 0xFC) >> 2;

            printvalue("SCID", 3, "Sub-channel Identifier", to_string(scid));
            sad[i] = (p[8+4*i] & 0x03) * 256uL + p[9+4*i];

            printvalue("SAD", 3, "Sub-channel Start Address", to_string(sad[i]));
            tpl = (p[10+4*i] & 0xFC) >> 2;

            if ((tpl & 0x20) >> 5 == 1) {
                uint8_t opt, plevel;
                string plevelstr;
                string rate;
                int num_cu = 0;
                opt = (tpl & 0x1c) >> 2;
                plevel = (tpl & 0x03);
                if (opt == 0x00) {
                    if (plevel == 0) {
                        plevelstr = "1-A";
                        rate = "1/4";
                        num_cu = 16;
                    }
                    else if (plevel == 1) {
                        plevelstr = "2-A";
                        rate = "3/8";
                        num_cu = 8;
                    }
                    else if (plevel == 2) {
                        plevelstr = "3-A";
                        rate = "1/2";
                        num_cu = 6;
                    }
                    else if (plevel == 3) {
                        plevelstr = "4-A";
                        rate = "3/4";
                        num_cu = 4;
                    }
                }
                else if (opt == 0x01) {
                    if (plevel == 0) {
                        plevelstr = "1-B";
                        rate = "4/9";
                        num_cu = 27;
                    }
                    else if (plevel == 1) {
                        plevelstr = "2-B";
                        rate = "4/7";
                        num_cu = 21;
                    }
                    else if (plevel == 2) {
                        plevelstr = "3-B";
                        rate = "4/6";
                        num_cu = 18;
                    }
                    else if (plevel == 3) {
                        plevelstr = "4-B";
                        rate = "4/5";
                        num_cu = 15;
                    }
                }
                else {
                    plevelstr = "Unknown option " + to_string(opt);
                }
                printvalue("TPL", 3, "Sub-channel Type and Protection Level");
                printvalue("EEP", 4, "Equal Error Protection", to_string(tpl));
                printvalue("Level", 5, "", plevelstr);
                if (not rate.empty()) {
                    printvalue("Rate", 5, "", rate);
                }
                if (num_cu) {
                    printvalue("CUs", 5, "", to_string(num_cu));
                }
            }
            else {
                uint8_t tsw, uepidx;
                tsw = (tpl & 0x08);
                uepidx = tpl & 0x07;
                printvalue("TPL", 3, "Sub-channel Type and Protection Level");
                printvalue("UEP", 4, "Unequal Error Protection", to_string(tpl));
                printvalue("Table switch", 5, "", to_string(tsw));
                printvalue("Index", 5, "", to_string(uepidx));
            }
            stl[i] = (p[10+4*i] & 0x03) * 256uL +
                      p[11+4*i];
            printvalue("STL", 3, "Sub-channel Stream Length", to_string(stl[i]));
            printvalue("bitrate", 3, "kbit/s", to_string(stl[i]*8/3));

            if (config.statistics and config.streams_to_decode.count(scid) == 0) {
                config.streams_to_decode.emplace(std::piecewise_construct,
                        std::make_tuple(scid),
                        std::make_tuple(scid, false)); // do not dump to file
            }

            if (config.streams_to_decode.count(scid) > 0) {
                config.streams_to_decode.at(scid).set_subchannel_index(stl[i]/3);
                config.streams_to_decode.at(scid).stream_index = i;
            }
        }

        // EOH
        printbuf("EOH", 1, p + 8 + 4*nst, 4, "End Of Header");
        uint16_t mnsc = read_u16_from_buf(p + (8 + 4*nst));
        printbuf("MNSC", 2, p+8+4*nst, 2, "Multiplex Network Signalling Channel", strprintf("%04x", mnsc));

        crch = read_u16_from_buf(p + (8 + 4*nst + 2));
        crc  = 0xffff;

        for (int i=4; i < 8 + 4*nst + 2; i++) {
            crc = update_crc_ccitt(crc, p[i]);
        }
        crc =~ crc;

        if (crc == crch) {
            sprintf(sdesc, "OK");
        }
        else {
            sprintf(sdesc, "Mismatch: %02x",crc);
        }

        printbuf("Header CRC", 2, p + 8 + 4*nst + 2, 2, "", sdesc);

        // MST - FIC
        if (ficf == 1) {
            uint8_t *fib, *fig;

            FIGalyser figs;

            uint8_t ficdata[32*4];
            memcpy(ficdata, p + 12 + 4*nst, ficl*4);
            printvalue("FIG Length", 1, "FIC length in bytes", to_string(ficl*4));
            printvalue("FIC", 1);
            fib = p + 12 + 4*nst;

            for (int i = 0; i < ficl*4/32; i++) {
                printsequencestart(2);
                printvalue("FIB", 3, "", to_string(i));
                fig=fib;
                figs.set_fib(i);
                rate_new_fib(i);

                const uint16_t figcrc = read_u16_from_buf(fib + 30);
                crc = 0xffff;
                for (int j = 0; j < 30; j++) {
                    crc = update_crc_ccitt(crc, fib[j]);
                }
                crc =~ crc;
                const bool crccorrect = (crc == figcrc);
                if (crccorrect)
                    printvalue("CRC", 3, "", "OK");
                else {
                    printvalue("CRC", 3, "",
                            strprintf("Mismatch: %04x %04x", crc, figcrc));
                }

                if (crccorrect or config.ignore_error) {
                    printvalue("FIGs", 3);

                    bool endmarker = false;
                    int figcount = 0;
                    while (!endmarker) {
                        uint8_t figtype, figlen;
                        figtype = (fig[0] & 0xE0) >> 5;
                        if (figtype != 7) {
                            figlen = fig[0] & 0x1F;

                            printsequencestart(4);
                            decodeFIG(config, figs, fig+1, figlen, figtype, 5, crccorrect);
                            fig += figlen + 1;
                            figcount += figlen + 1;
                            if (figcount >= 29)
                                endmarker = true;
                        }
                        else {
                            endmarker = true;
                        }
                    }
                }

                fib += 32;
            }

            if (config.analyse_fic_carousel) {
                figs.analyse(get_mode_identity());
            }
        }

        printvalue("Stream Data", 1);
        int offset = 0;
        for (int i=0; i < nst; i++) {
            uint8_t streamdata[684*8];
            memcpy(streamdata, p + 12 + 4*nst + ficf*ficl*4 + offset, stl[i]*8);
            offset += stl[i] * 8;
            printsequencestart(2);
            printvalue("Id", 3, "", to_string(i));
            printvalue("Length", 3, "", to_string(stl[i]*8));

            int subchid = -1;
            for (const auto& el : config.streams_to_decode) {
                if (el.second.stream_index == i) {
                    subchid = el.first;
                    break;
                }
            }
            printvalue("Selected for decoding", 3, "", (subchid == -1 ? "false" : "true"));

            if (get_verbosity() > 1) {
                printbuf("Data", 3, streamdata, stl[i]*8);
            }

            if (subchid != -1) {
                config.streams_to_decode.at(subchid).push(streamdata, stl[i]*8);
            }
        }

        //* EOF (4 Bytes)
        printbuf("EOF", 1, p + 12 + 4*nst + ficf*ficl*4 + offset, 4);

        // CRC (2 Bytes)
        crch = read_u16_from_buf(p + (12 + 4*nst + ficf*ficl*4 + offset));
        crc = 0xffff;

        for (int i = 12 + 4*nst; i < 12 + 4*nst + ficf*ficl*4 + offset; i++) {
            crc = update_crc_ccitt(crc, p[i]);
        }
        crc =~ crc;
        if (crc == crch)
            sprintf(sdesc, "OK");
        else
            sprintf(sdesc, "Mismatch: %02x", crc);

        printbuf("CRC", 2, p + 12 + 4*nst + ficf*ficl*4 + offset, 2, "", sdesc);

        // RFU (2 Bytes)
        printbuf("RFU", 2, p + 12 + 4*nst + ficf*ficl*4 + offset + 2, 2);

        //* TIST (4 Bytes)
        const size_t tist_ix = 12 + 4*nst + ficf*ficl*4 + offset + 4;
        uint32_t TIST = (uint32_t)(p[tist_ix]) << 24 |
                        (uint32_t)(p[tist_ix+1]) << 16 |
                        (uint32_t)(p[tist_ix+2]) << 8 |
                        (uint32_t)(p[tist_ix+3]);

        sprintf(sdesc, "%f", (TIST & 0xFFFFFF) / 16384.0);
        printbuf("TIST", 1, p + tist_ix, 4, "Time Stamp (ms)", sdesc);

        if (config.analyse_fig_rates and (fct % 250) == 0) {
            rate_display_analysis(false, config.analyse_fig_rates_per_second);
        }

        num_frames++;
        if (config.num_frames_to_decode > 0 and
                num_frames >= config.num_frames_to_decode) {
            fprintf(stderr, "Decoded %zu ETI frames\n", num_frames);
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
        fprintf(stat_fd, "    label: %s\n", ensemble.label.label().c_str());
        fprintf(stat_fd, "    shortlabel: %s\n", ensemble.label.shortlabel().c_str());
        fprintf(stat_fd, "audio:\n");

        for (const auto& snoop : config.streams_to_decode) {
            bool corresponding_service_found = false;

            for (const auto& service : ensemble.services) {
                for (const auto& component : service.components) {
                    if (component.subchId == snoop.first and component.primary) {
                        corresponding_service_found = true;
                        fprintf(stat_fd, "    - service_id: 0x%x\n", service.id);
                        fprintf(stat_fd, "      subchannel_id: 0x%x\n", component.subchId);
                        fprintf(stat_fd, "      label: %s\n", service.label.label().c_str());
                        fprintf(stat_fd, "      shortlabel: %s\n", service.label.shortlabel().c_str());
                        fprintf(stat_fd, "      extended_label: %s\n", service.label.assemble().c_str());

                        try {
                            const auto& subch = ensemble.get_subchannel(component.subchId);
                            fprintf(stat_fd, "      subchannel:\n");
                            fprintf(stat_fd, "          id: %d\n", subch.id);
                            fprintf(stat_fd, "          SAd: %d\n", subch.start_addr);

                            using ensemble_database::subchannel_t;
                            switch (subch.protection_type) {
                                case subchannel_t::protection_type_t::EEP:
                                    switch (subch.protection_option) {
                                        case subchannel_t::protection_eep_option_t::EEP_A:
                                            fprintf(stat_fd, "          protection: EEP %d-A\n",
                                                    subch.protection_level + 1);
                                            break;
                                        case subchannel_t::protection_eep_option_t::EEP_B:
                                            fprintf(stat_fd, "          protection: EEP %d-B\n",
                                                    subch.protection_level + 1);
                                            break;
                                        default:
                                            fprintf(stat_fd, "          protection: unknown\n");
                                            break;
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
        printf("Watermark: %s\n", watermark.c_str());
    }

    if (config.analyse_fig_rates) {
        rate_display_analysis(false, config.analyse_fig_rates_per_second);
    }

    figs_cleardb();
}

void ETI_Analyser::fic_analyse()
{
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

    bool running = true;
    int i = 0;
    while (running) {
        FIGalyser figs;
        uint8_t fib[32];
        if (fread(fib, 32, 1, config.ficfd) == 0) {
            break;
        }

        printf("---\n");
        printvalue("LIDATA", 0);
        printvalue("FIC", 1);
        printsequencestart(2);
        printvalue("FIB", 3, "", to_string(i));
        figs.set_fib(i);
        rate_new_fib(i);

        const uint16_t figcrc = read_u16_from_buf(fib + 30);
        uint16_t crc = 0xffff;
        for (int j = 0; j < 30; j++) {
            crc = update_crc_ccitt(crc, fib[j]);
        }
        crc =~ crc;
        const bool crccorrect = (crc == figcrc);
        if (crccorrect)
            printvalue("CRC", 3, "", "OK");
        else {
            printvalue("CRC", 3, "",
                    strprintf("Mismatch: %04x %04x", crc, figcrc));
        }

        if (crccorrect or config.ignore_error) {
            printvalue("FIGs", 3);

            uint8_t *fig = fib;
            bool endmarker = false;
            int figcount = 0;
            while (!endmarker) {
                uint8_t figtype, figlen;
                figtype = (fig[0] & 0xE0) >> 5;
                if (figtype != 7) {
                    figlen = fig[0] & 0x1F;

                    printsequencestart(4);
                    decodeFIG(config, figs, fig+1, figlen, figtype, 5, crccorrect);
                    fig += figlen + 1;
                    figcount += figlen + 1;
                    if (figcount >= 29)
                        endmarker = true;
                }
                else {
                    endmarker = true;
                }
            }
        }

        if (quit.load()) running = false;

        if (config.analyse_fic_carousel) {
            figs.analyse(1);
        }

        i = (i+1) % 3;
    }
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
    switch (figtype) {
        case 0:
            {
                fig0_common_t fig0(f, figlen, ensemble, wm_decoder);
                fig0.fibcrccorrect = fibcrccorrect;

                const display_settings_t disp(config.is_fig_to_be_printed(figtype, fig0.ext()), indent);

                printvalue("FIG", disp, "", strprintf("0/%d", fig0.ext()));
                if (get_verbosity() > 0) {
                    printbuf("Data", disp, f, figlen);
                }

                if (disp.print) {
                    printvalue("Length", disp, "", to_string(figlen));
                    printvalue("OE", disp, "", to_string(fig0.oe()));
                    printvalue("C/N", disp, "", to_string(fig0.cn()));
                    printvalue("P/D", disp, "", to_string(fig0.pd()));
                }

                figs.push_back(figtype, fig0.ext(), figlen);

                auto fig_result = fig0_select(fig0, disp);
                fig_result.figtype = figtype;
                fig_result.figext = fig0.ext();
                printvalue("Decoding", disp);
                print_fig_result(fig_result, disp+1);

                rate_announce_fig(figtype, fig0.ext(), fig_result.complete);
            }
            break;

        case 1:
            {// SHORT LABELS
                fig1_common_t fig1(ensemble, f, figlen);
                fig1.fibcrccorrect = fibcrccorrect;

                const display_settings_t disp(config.is_fig_to_be_printed(figtype, fig1.ext()), indent);

                printvalue("FIG", disp, "", strprintf("1/%d", fig1.ext()));
                if (get_verbosity() > 0) {
                    printbuf("Data", disp, f, figlen);
                }

                if (disp.print) {
                    printvalue("Length", disp, "", to_string(figlen));
                    printvalue("OE", disp, "", to_string(fig1.oe()));
                }

                figs.push_back(figtype, fig1.ext(), figlen);

                auto fig_result = fig1_select(fig1, disp);
                fig_result.figtype = figtype;
                fig_result.figext = fig1.ext();
                printvalue("Decoding", disp);
                print_fig_result(fig_result, disp+1);
                rate_announce_fig(figtype, fig1.ext(), fig_result.complete);
            }
            break;
        case 2:
            {// EXTENDED LABELS
                fig2_common_t fig2(ensemble, f, figlen);
                const display_settings_t disp(config.is_fig_to_be_printed(figtype, fig2.ext()), indent);
                auto fig_result = fig2_select(fig2, disp);

                printvalue("FIG", disp, "", strprintf("2/%d", fig2.ext()));

                if (get_verbosity() > 0) {
                    printbuf("Data", disp, f, figlen);
                }

                if (disp.print) {
                    printvalue("Length", disp, "", to_string(figlen));
                }

                figs.push_back(figtype, fig2.ext(), figlen);

                printvalue("Decoding", disp);
                print_fig_result(fig_result, disp+1);
                rate_announce_fig(figtype, fig2.ext(), fig_result.complete);
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

                printvalue("FIG", disp, "", strprintf("5/%d", ext));

                if (get_verbosity() > 0) {
                    printbuf("Data", disp, f, figlen);
                }

                if (disp.print) {
                    printvalue("Length", disp, "", to_string(figlen));
                    printvalue("D1", disp, "", to_string(d1));
                    printvalue("D2", disp, "", to_string(d2));
                    printvalue("TCId", disp, "", to_string(tcid));
                }

                figs.push_back(figtype, ext, figlen);

                bool complete = true; // TODO verify
                rate_announce_fig(figtype, ext, complete);
            }
            break;
        case 6:
            {// Conditional access
                fprintf(stderr, "ERROR: ETI contains unsupported FIG 6\n");
                printvalue("FIG", indent, "", "6 - unsupported");
            }
            break;
        default:
            {
                fprintf(stderr, "ERROR: ETI contains unknown FIG %d\n", figtype);
                printvalue("FIG", indent, "", strprintf("%d - unsupported", figtype));
            }
            break;
    }
}
