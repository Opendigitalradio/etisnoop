/*
    Copyright (C) 2014 CSP Innovazione nelle ICT s.c.a r.l. (http://www.csp.it/)
    Copyright (C) 2019 Matthias P. Braendli (http://www.opendigitalradio.org)
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

    Ensemble Database, gather data from the ensemble for the
    statistics.

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

#include <stdexcept>
#include <string>
#include <list>
#include <vector>
#include <map>

namespace ensemble_database {

enum class extended_label_charset {
    UTF8, // encoding flag = 0
    UCS2, // encoding flag = 1
};

struct label_t {
    // FIG 1 Label and shortlabel
    std::string label;
    uint16_t shortlabel_flag;

    // Extended Label from FIG 2
    std::map<int, std::vector<uint8_t> > segments;
    size_t segment_count = 0; // number if actual segments (not segment count as in spec)
    extended_label_charset charset;
    uint8_t toggle_flag = 0;

    // Assemble all segments into a UTF-8 string. Returns an
    // empty string if not all segments received.
    std::string assemble() const;

    // Return a string that represents segment count and completeness
    std::string assembly_state() const;
};

struct subchannel_t {
    uint8_t id;
    uint8_t start_addr;

    enum class protection_type_t { UEP, EEP };

    protection_type_t protection_type;

    // Long form FIG0/1, i.e. EEP
    enum class protection_eep_option_t { EEP_A, EEP_B };
    protection_eep_option_t protection_option;
    int protection_level;
    int size;

    // Short form FIG0/1, i.e. UEP
    int table_switch;
    int table_index;

    // TODO type bitrate
};

struct component_t {
    uint32_t service_id;
    uint8_t subchId;

    bool primary;

    label_t label;

    /* TODO
    uint8_t type;

    uaType for audio
    */
};

struct service_t {
    uint32_t id;
    label_t label;

    bool programme_not_data;

    std::list<component_t> components;

    component_t& get_component(uint32_t subchannel_id);
    component_t& get_or_create_component(uint32_t subchannel_id);

    // TODO PTy language announcement
};


class not_found : public std::runtime_error
{
    public:
        not_found(const std::string& msg) : std::runtime_error(msg) {}
};

struct ensemble_t {
    uint16_t EId;
    label_t label;

    std::list<service_t> services;
    std::list<subchannel_t> subchannels;

    // TODO ecc

    service_t& get_service(uint32_t service_id);
    service_t& get_or_create_service(uint32_t service_id);

    subchannel_t& get_subchannel(uint8_t subchannel_id);
    subchannel_t& get_or_create_subchannel(uint8_t subchannel_id);
};

}

