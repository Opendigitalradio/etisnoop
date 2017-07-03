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

#include <string>
#include <list>

struct service_t {
    uint32_t id;
    std::string label;
    std::string shortlabel;
    // TODO PTy language announcement
};

struct subchannel_t {
    uint8_t address;
    uint8_t num_cu;
    // TODO type bitrate protection
};

struct component_t {
    uint32_t service_id;
    uint8_t subchId;
    /* TODO
    uint8_t type;
    uint8_t SCIdS;

    uaType for audio
    */
};

struct ensemble_t {
    std::list<service_t> services;
    std::list<subchannel_t> subchannels;
    std::list<component_t> components;
};

