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

    etisnoop.cpp
          Parse ETI NI G.703 file

    Authors:
         Sergio Sagliocco <sergio.sagliocco@csp.it>
         Matthias P. Braendli <matthias@mpb.li>
                   / |  |-  ')|)  |-|_ _ (|,_   .|  _  ,_ \
         Data Path \(|(||_(|/_| (||_||(a)_||||(|||.(_()|||/

*/

#pragma once

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define ETINIPACKETSIZE 6144

#include <vector>
#include <map>
#include <list>
#include <atomic>
#include "dabplussnoop.hpp"
#include "watermarkdecoder.hpp"
#include "repetitionrate.hpp"
#include "figalyser.hpp"

extern std::atomic<bool> quit;

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
    std::map<int, StreamSnoop> streams_to_decode;
    std::list<std::pair<int, int> > figs_to_display;
    bool analyse_fic_carousel;
    bool analyse_fig_rates;
    bool analyse_fig_rates_per_second;
    bool decode_watermark;

    bool is_fig_to_be_printed(int type, int extension) const;
};


int eti_analyse(eti_analyse_config_t &config);

void decodeFIG(
        const eti_analyse_config_t &config,
        FIGalyser &figs,
        WatermarkDecoder &wm_decoder,
        uint8_t* f,
        uint8_t figlen,
        uint16_t figtype,
        int indent);

