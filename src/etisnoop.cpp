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

    etisnoop.cpp
          Parse ETI NI G.703 file

    Authors:
         Sergio Sagliocco <sergio.sagliocco@csp.it>
         Matthias P. Braendli <matthias@mpb.li>
                   / |  |-  ')|)  |-|_ _ (|,_   .|  _  ,_ \
         Data Path \(|(||_(|/_| (||_||(a)_||||(|||.(_()|||/

*/


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <cinttypes>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <time.h>
extern "C" {
#include "lib_crc.h"
}
#include "dabplussnoop.hpp"
#include "utils.hpp"
#include "etiinput.hpp"
#include "figs.hpp"
#include "watermarkdecoder.hpp"
#include "repetitionrate.hpp"

struct FIG
{
    int type;
    int ext;
    int len;
};

class FIGalyser
{
    public:
        FIGalyser()
        {
            clear();
        }

        void set_fib(int fib)
        {
            m_fib = fib;
        }

        void push_back(int type, int ext, int len)
        {
            struct FIG fig = {
                .type = type,
                .ext  = ext,
                .len  = len };

            m_figs[m_fib].push_back(fig);
        }

        void analyse()
        {
            printf("FIC ");

            for (size_t fib = 0; fib < m_figs.size(); fib++) {
                int consumed = 7;
                int fic_size = 0;
                printf("[%1zu ", fib);

                for (size_t i = 0; i < m_figs[fib].size(); i++) {
                    FIG &f = m_figs[fib][i];
                    printf("%01d/%02d (%2d) ", f.type, f.ext, f.len);

                    consumed += 10;

                    fic_size += f.len;
                }

                printf(" ");

                int align = 60 - consumed;
                if (align > 0) {
                    while (align--) {
                        printf(" ");
                    }
                }

                printf("|");

                for (int i = 0; i < 15; i++) {
                    if (2*i < fic_size) {
                        printf("#");
                    }
                    else {
                        printf("-");
                    }
                }

                printf("| ]   ");

            }

            printf("\n");
        }

        void clear()
        {
            m_figs.clear();
            m_figs.resize(3);
        }

    private:
        int m_fib;
        std::vector<std::vector<FIG> > m_figs;
};


struct FIG0_13_shortAppInfo
{
    uint16_t SId;
    uint8_t No:4;
    uint8_t SCIdS:4;
} PACKED;


#define ETINIPACKETSIZE 6144

using namespace std;

struct eti_analyse_config_t {
    eti_analyse_config_t() :
        etifd(nullptr),
        ignore_error(false),
        streams_to_decode(),
        analyse_fic_carousel(false),
        analyse_fig_rates(false),
        analyse_fig_rates_per_second(false),
        decode_watermark(false) {}

    FILE* etifd;
    bool ignore_error;
    std::map<int, DabPlusSnoop> streams_to_decode;
    bool analyse_fic_carousel;
    bool analyse_fig_rates;
    bool analyse_fig_rates_per_second;
    bool decode_watermark;
};


// Function prototypes
void decodeFIG(FIGalyser &figs,
               WatermarkDecoder &wm_decoder,
               uint8_t* figdata,
               uint8_t figlen,
               uint16_t figtype,
               int indent);

int eti_analyse(eti_analyse_config_t& config);

const char *get_programme_type_str(size_t int_table_Id, size_t pty);
int sprintfMJD(char *dst, int mjd);

#define no_argument 0
#define required_argument 1
#define optional_argument 2
const struct option longopts[] = {
    {"help",               no_argument,        0, 'h'},
    {"verbose",            no_argument,        0, 'v'},
    {"ignore-error",       no_argument,        0, 'e'},
    {"decode-stream",      required_argument,  0, 'd'},
    {"input",              required_argument,  0, 'i'}
};

void usage(void)
{
    fprintf(stderr,
            "Opendigitalradio ETISnoop analyser %s compiled at %s, %s\n\n"
            "The ETISnoop analyser decodes and prints out a RAW ETI file in a\n"
            "form that makes analysis easier.\n"
            "\n"
            "  http://www.opendigitalradio.org\n"
            "\n"
            "Usage: etisnoop [-v] [-f] [-w] [-i filename] [-d stream_index]\n"
            "\n"
            "   -v      increase verbosity (can be given more than once)\n"
            "   -d N    decode subchannel N into .msc and .wav files\n"
            "   -f      analyse FIC carousel\n"
            "   -r      analyse FIG rates in FIGs per second\n"
            "   -R      analyse FIG rates in frames per FIG\n"
            "   -w      decode CRC-DABMUX and ODR-DabMux watermark.\n",
#if defined(GITVERSION)
            GITVERSION,
#else
            VERSION,
#endif
            __DATE__, __TIME__);
}

int main(int argc, char *argv[])
{
    int index;
    int ch = 0;
    string file_name("-");

    eti_analyse_config_t config;

    while(ch != -1) {
        ch = getopt_long(argc, argv, "d:efhrRvwi:", longopts, &index);
        switch (ch) {
            case 'd':
                {
                int subchix = atoi(optarg);
                DabPlusSnoop dps;
                config.streams_to_decode[subchix] = dps;
                }
                break;
            case 'e':
                config.ignore_error = true;
                break;
            case 'i':
                file_name = optarg;
                break;
            case 'f':
                config.analyse_fic_carousel = true;
                break;
            case 'r':
                config.analyse_fig_rates = true;
                config.analyse_fig_rates_per_second = true;
                break;
            case 'R':
                config.analyse_fig_rates = true;
                config.analyse_fig_rates_per_second = false;
                break;
            case 'v':
                set_verbosity(get_verbosity() + 1);
                break;
            case 'w':
                config.decode_watermark = true;
                break;
            case 'h':
                usage();
                return 1;
                break;
        }
    }

    FILE* etifd;

    if (file_name == "-") {
        printf("Analysing stdin\n");
        etifd = stdin;
    }
    else {
        etifd = fopen(file_name.c_str(), "r");
        if (etifd == NULL) {
            perror("File open failed");
            return 1;
        }
    }
    config.etifd = etifd;

    eti_analyse(config);
    fclose(etifd);
}

int eti_analyse(eti_analyse_config_t& config)
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

    bool running = true;

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

    WatermarkDecoder wm_decoder;

    if (config.analyse_fig_rates) {
        rate_display_header(config.analyse_fig_rates_per_second);
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

            if (config.streams_to_decode.count(i) > 0) {
                config.streams_to_decode[i].set_subchannel_index(stl[i]/3);
                config.streams_to_decode[i].set_index(i);
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
            int endmarker = 0;
            int figcount = 0;
            uint8_t *fib, *fig;
            uint16_t figcrc;

            FIGalyser figs;

            uint8_t ficdata[32*4];
            memcpy(ficdata, p + 12 + 4*nst, ficl*4);
            sprintf(sdesc, "FIC Data (%d bytes)", ficl*4);
            //printbuf(sdesc, 1, ficdata, ficl*4);
            printbuf(sdesc, 1, NULL, 0);
            fib = p + 12 + 4*nst;
            for(int i = 0; i < ficl*4/32; i++) {
                sprintf(sdesc, "FIB %d", i);
                printbuf(sdesc, 1, NULL, 0);
                fig=fib;
                figs.set_fib(i);
                rate_new_fib(i);
                endmarker=0;
                figcount=0;
                while (!endmarker) {
                    uint8_t figtype, figlen;
                    figtype = (fig[0] & 0xE0) >> 5;
                    if (figtype != 7) {
                        figlen = fig[0] & 0x1F;
                        sprintf(sdesc, "FIG %d [%d bytes]", figtype, figlen);
                        printbuf(sdesc, 3, fig+1, figlen);
                        decodeFIG(figs, wm_decoder, fig+1, figlen, figtype, 4);
                        fig += figlen + 1;
                        figcount += figlen + 1;
                        if (figcount >= 29)
                            endmarker = 1;
                    }
                    else {
                        endmarker = 1;
                    }
                }
                figcrc = fib[30]*256 + fib[31];
                crc = 0xffff;
                for (int j = 0; j < 30; j++) {
                    crc = update_crc_ccitt(crc, fib[j]);
                }
                crc =~ crc;
                if (crc == figcrc)
                    sprintf(sdesc, "FIB %d CRC OK", i);
                else
                    sprintf(sdesc, "FIB %d CRC Mismatch: %02x", i, crc);

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
            if (get_verbosity() > 1) {
                printbuf("Stream Data", 1, streamdata, stl[i]*8, sdesc);
            }
            else {
                printbuf("Stream Data", 1, streamdata, 0, sdesc);
            }

            if (config.streams_to_decode.count(i) > 0) {
                config.streams_to_decode[i].push(streamdata, stl[i]*8);
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
    }


    std::map<int, DabPlusSnoop>::iterator it;
    for (it = config.streams_to_decode.begin();
            it != config.streams_to_decode.end();
            ++it) {
        it->second.close();
    }

    if (config.decode_watermark) {
        std::string watermark(wm_decoder.calculate_watermark());
        printf("Watermark:\n  %s\n", watermark.c_str());
    }

    if (config.analyse_fig_rates) {
        rate_display_analysis(false, config.analyse_fig_rates_per_second);
    }

    figs_cleardb();


    return 0;
}

void decodeFIG(FIGalyser &figs,
               WatermarkDecoder &wm_decoder,
               uint8_t* f,
               uint8_t figlen,
               uint16_t figtype,
               int indent)
{
    char desc[512];

    switch (figtype) {
        case 0:
            {
                fig0_common_t fig0(f, figlen, wm_decoder);

                sprintf(desc, "FIG %d/%d: C/N=%d OE=%d P/D=%d",
                        figtype, fig0.ext(), fig0.cn(), fig0.oe(), fig0.pd());
                printbuf(desc, indent, f+1, figlen-1);

                figs.push_back(figtype, fig0.ext(), figlen);

                bool complete = fig0_select(fig0, indent);
                rate_announce_fig(figtype, fig0.ext(), complete);
            }
            break;

        case 1:
            {// SHORT LABELS
                fig1_common_t fig1(f, figlen);

                figs.push_back(figtype, fig1.ext(), figlen);

                bool complete = fig1_select(fig1, indent);
                rate_announce_fig(figtype, fig1.ext(), complete);
            }
            break;
        case 2:
            {// LONG LABELS
                uint16_t ext,oe;

                uint8_t toggle_flag = (f[0] & 0x80) >> 7;
                uint8_t segment_index = (f[0] & 0x70) >> 4;
                oe = (f[0] & 0x08) >> 3;
                ext = f[0] & 0x07;
                sprintf(desc,
                        "FIG %d/%d: Toggle flag=%d, Segment_index=%d, OE=%d",
                        figtype, ext, toggle_flag, segment_index, oe);

                printbuf(desc, indent, f+1, figlen-1);

                figs.push_back(figtype, ext, figlen);

                bool complete = true; // TODO verify
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
                sprintf(desc,
                        "FIG %d/%d: D1=%d, D2=%d, TCId=%d",
                        figtype, ext, d1, d2, tcid);

                printbuf(desc, indent, f+1, figlen-1);

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


