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

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "utils.hpp"
#include "tables.hpp"
#include "watermarkdecoder.hpp"
#include "ensembledatabase.hpp"

struct fig_result_t {
    struct msg_info_t {
        msg_info_t(int level_, const std::string& msg_) :
            level(level_), msg(msg_) {}
        msg_info_t(const std::string& msg_) :
            level(0), msg(msg_) {}
        int level = 0;
        std::string msg;
    };

    int figtype = -1;
    int figext = 0;

    std::vector<msg_info_t> msgs;
    std::vector<std::string> errors;
    bool complete = false;
};

void figs_cleardb(void);

struct fig0_common_t {
    fig0_common_t(
            uint8_t* fig_data,
            uint16_t fig_len,
            ensemble_database::ensemble_t &ens,
            WatermarkDecoder &wm_dec) :
        f(fig_data),
        figlen(fig_len),
        ensemble(ens),
        fibcrccorrect(true),
        wm_decoder(wm_dec) { }

    uint8_t* f;
    uint16_t figlen;
    ensemble_database::ensemble_t& ensemble;
    // The ensemble only gets updated when the fib crc is ok
    bool fibcrccorrect;
    WatermarkDecoder &wm_decoder;

    uint16_t cn(void) { return (f[0] & 0x80) >> 7; }
    uint16_t oe(void) { return (f[0] & 0x40) >> 6; }
    uint16_t pd(void) { return (f[0] & 0x20) >> 5; }
    uint16_t ext(void) { return f[0] & 0x1F; }
};

struct fig1_common_t {
    fig1_common_t(
            ensemble_database::ensemble_t &ens,
            uint8_t* fig_data,
            uint16_t fig_len) :
        fibcrccorrect(true),
        ensemble(ens),
        f(fig_data),
        figlen(fig_len) {}

    // The ensemble only gets updated when the fib crc is ok
    bool fibcrccorrect;
    ensemble_database::ensemble_t& ensemble;

    uint8_t* f;
    uint16_t figlen;

    uint8_t charset() { return (f[0] & 0xF0) >> 4; }
    uint8_t oe() { return (f[0] & 0x08) >> 3; }
    uint8_t ext() { return f[0] & 0x07; }
};

// FIG 0/11 and 0/22 struct
struct Lat_Lng {
    double latitude, longitude;
};

// Which international table has been chosen
size_t get_international_table(void);
void   set_international_table(size_t intl_table);

// MID is used by some FIGs. It is signalled in LIDATA - FC - MID
void set_mode_identity(uint8_t mid);
uint8_t get_mode_identity();

fig_result_t fig0_select(fig0_common_t& fig0, const display_settings_t &disp);

fig_result_t fig0_0(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_1(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_2(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_3(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_5(fig0_common_t& fig0, const display_settings_t &disp);
void fig0_6_cleardb();
fig_result_t fig0_6(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_8(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_9(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_10(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_11(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_13(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_14(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_16(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_17(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_18(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_19(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_21(fig0_common_t& fig0, const display_settings_t &disp);
void fig0_22_cleardb();
fig_result_t fig0_22(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_24(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_25(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_26(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_27(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_28(fig0_common_t& fig0, const display_settings_t &disp);
fig_result_t fig0_31(fig0_common_t& fig0, const display_settings_t &disp);

fig_result_t fig1_select(fig1_common_t& fig1, const display_settings_t &disp);

