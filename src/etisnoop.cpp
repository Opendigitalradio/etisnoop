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
#include <regex>
#include <sstream>
#include <time.h>
#include <signal.h>

#include "etianalyse.hpp"
#include "dabplussnoop.hpp"
#include "utils.hpp"
#include "etiinput.hpp"
#include "figs.hpp"
#include "watermarkdecoder.hpp"
#include "repetitionrate.hpp"
#include "figalyser.hpp"

using namespace std;

static void handle_signal(int)
{
    quit.store(true);
}

#define no_argument 0
#define required_argument 1
#define optional_argument 2
const struct option longopts[] = {
    {"analyse-figs",       no_argument,        0, 'f'},
    {"decode-stream",      required_argument,  0, 'd'},
    {"filter-fig",         required_argument,  0, 'F'},
    {"help",               no_argument,        0, 'h'},
    {"ignore-error",       no_argument,        0, 'e'},
    {"input",              required_argument,  0, 'i'},
    {"input-fic",          required_argument,  0, 'I'},
    {"num-frames",         required_argument,  0, 'n'},
    {"statistics",         required_argument,  0, 's'},
    {"verbose",            no_argument,        0, 'v'},
};

void usage(void)
{
    fprintf(stderr,
            "Opendigitalradio ETISnoop analyser %s compiled at %s, %s\n\n"
            "The ETISnoop analyser decodes a RAW ETI file and prints out\n"
            "its contents in YAML for easier analysis.\n"
            "\n"
            "It can also read a FIC dump from file.\n"
            "\n"
            "  http://www.opendigitalradio.org\n"
            "\n"
            "Usage: etisnoop [options] [(-i|-I) filename]\n"
            "\n"
            "   -i      the file contains RAW ETI\n"
            "   -I      the file contains FIC\n"
            "   -v      increase verbosity (can be given more than once)\n"
            "   -d N    write subchannel N into stream-N.dab\n"
            "           (superframes with RS coding)\n"
            "           if the suchannel contains DAB+ audio will be decoded to stream-N.wav\n"
            "   -s <filename.yaml>\n"
            "           statistics mode: decode all subchannels and measure audio level, write statistics to file\n"
            "   -n N    stop analysing after N ETI frames\n"
            "   -f      analyse FIC carousel (no YAML output)\n"
            "   -r      analyse FIG rates in FIGs per second\n"
            "   -R      analyse FIG rates in frames per FIG\n"
            "   -w      decode CRC-DABMUX and ODR-DabMux watermark.\n"
            "   -e      decode frames with SYNC error and decode FIGs with invalid CRC\n"
            "   -F <type>/<ext>\n"
            "           add FIG type/ext to list of FIGs to display.\n"
            "           if the option is not given, all FIGs are displayed.\n"
            "\n",
#if defined(GITVERSION)
            GITVERSION,
#else
            VERSION,
#endif
            __DATE__, __TIME__);
}

int main(int argc, char *argv[])
{
    struct sigaction sa;
    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = handle_signal;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    int index;
    int ch = 0;
    string file_name("-");
    bool file_contains_eti = false;
    bool file_contains_fic = false;

    eti_analyse_config_t config;

    while(ch != -1) {
        ch = getopt_long(argc, argv, "d:efF:hi:I:n:rRs:vw", longopts, &index);
        switch (ch) {
            case 'd':
                {
                int subchid = atoi(optarg);
                config.streams_to_decode.emplace(std::piecewise_construct,
                        std::make_tuple(subchid),
                        std::make_tuple(subchid, true)); // dump to file
                }
                break;
            case 'e':
                config.ignore_error = true;
                break;
            case 'f':
                config.analyse_fic_carousel = true;
                break;
            case 'F':
                {
                const string type_ext(optarg);
                const std::regex regex("^([0-9]+)/([0-9]+)$");
                std::smatch match;
                bool is_match = std::regex_search(type_ext, match, regex);
                if (not is_match) {
                    fprintf(stderr, "Incorrect -F format\n");
                    return 1;
                }

                const string type_str = match[1];
                const int type = std::atoi(type_str.c_str());
                const string extension_str = match[2];
                const int extension = std::atoi(extension_str.c_str());

                fprintf(stderr, "Adding FIG %d/%d to filter\n", type, extension);
                config.figs_to_display.emplace_back(type, extension);
                }
                break;
            case 'i':
                file_name = optarg;
                file_contains_eti = true;
                break;
            case 'I':
                file_name = optarg;
                file_contains_fic = true;
                break;
            case 'n':
                config.num_frames_to_decode = std::atoi(optarg);
                break;
            case 'r':
                config.analyse_fig_rates = true;
                config.analyse_fig_rates_per_second = true;
                break;
            case 'R':
                config.analyse_fig_rates = true;
                config.analyse_fig_rates_per_second = false;
                break;
            case 's':
                config.statistics = true;
                config.statistics_filename = optarg;
                break;
            case 'v':
                set_verbosity(get_verbosity() + 1);
                break;
            case 'w':
                config.decode_watermark = true;
                break;
            case -1:
                break;
            default:
            case 'h':
                usage();
                return 1;
                break;
        }
    }


    if (file_contains_eti and file_contains_fic) {
        fprintf(stderr, "-i and -I are mutually exclusive\n");
        return 1;
    }
    else if (file_contains_eti or file_contains_fic) {
        FILE* fd;
        if (file_name == "-") {
            fprintf(stderr, "Analysing stdin\n");
            fd = stdin;
        }
        else {
            fd = fopen(file_name.c_str(), "r");
            if (fd == NULL) {
                perror("File open failed");
                return 1;
            }
        }

        if (file_contains_eti) {
            config.etifd = fd;
        }
        else {
            config.ficfd = fd;
        }

        ETI_Analyser eti_analyser(config);
        eti_analyser.analyse();
        fclose(fd);
    }
    else {
        fprintf(stderr, "Must specify either -i or -I\n");
        return 1;
    }
}

