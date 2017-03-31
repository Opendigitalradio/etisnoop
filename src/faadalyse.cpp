/*
    Copyright (C) 2017 Matthias P. Braendli (http://www.opendigitalradio.org)

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

    faadalyse.cpp
        Use FAAD to analyse some AAC file internals

    Authors:
         Matthias P. Braendli <matthias@mpb.li>
*/


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <getopt.h>

extern "C" {
#include "lib_crc.h"
}
#include "dabplussnoop.hpp"

using namespace std;

static void usage(void)
{
    cerr << "\nUsage:\n"
        "faadalyse <file.dabp> <bitrate>\n"
        "  The bitrate of the file must be given separately\n";
}

int main(int argc, char **argv)
{
    cerr << "Faadalyse -- A .dabp file analyser\n  www.opendigitalradio.org\n" <<
            " compiled at " << __DATE__ << " " << __TIME__ << "\n";

    if (argc != 3) {
        usage();
        return 1;
    }
    const string fname = argv[1];
    const int bitrate = stoi(argv[2]);
    if (bitrate % 8 != 0) {
        cerr << "Bitrate not a multiple of 8\n";
        return 1;
    }
    const int subchannel_index = bitrate / 8;

    cerr << "Analysing " << fname << " " << bitrate << "kbps\n";

    DabPlusSnoop snoop;
    snoop.set_subchannel_index(subchannel_index);

    FILE* fd = fopen(fname.c_str(), "r");
    if (!fd) {
        cerr << "Failed to open file\n";
        return 1;
    }

    while (!feof(fd) and !ferror(fd)) {
        vector<uint8_t> buf(1024);
        size_t read_bytes = fread(buf.data(), 1, 1024, fd);

        if (read_bytes) {
            snoop.push(buf.data(), read_bytes);
        }
    }

    snoop.close();

    cerr << "Write file stream-0.wav\n";

    return 0;
}

