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
#include "ensembledatabase.hpp"

extern std::atomic<bool> quit;

struct eti_analyse_config_t {
    FILE* etifd = nullptr;
    FILE* ficfd = nullptr;
    bool ignore_error = false;
    std::map<int /* subch index */, StreamSnoop> streams_to_decode;
    std::list<std::pair<int, int> > figs_to_display;
    bool analyse_fic_carousel = false;
    bool analyse_fig_rates = false;
    bool analyse_fig_rates_per_second = false;
    bool decode_watermark = false;
    bool statistics = false;
    std::string statistics_filename;
    size_t num_frames_to_decode = 0; // 0 means forever

    bool is_fig_to_be_printed(int type, int extension) const;
};

class ETI_Analyser {
    public:
        ETI_Analyser(eti_analyse_config_t &config) :
            config(config),
            ensemble(),
            wm_decoder() {}

        void analyse(void);

    private:
        void eti_analyse(void);
        void fic_analyse(void);

        void decodeFIG(
                const eti_analyse_config_t &config,
                FIGalyser &figs,
                uint8_t* f,
                uint8_t figlen,
                uint16_t figtype,
                int indent,
                bool fibcrccorrect);

        eti_analyse_config_t &config;

        ensemble_database::ensemble_t ensemble;
        WatermarkDecoder wm_decoder;
};

