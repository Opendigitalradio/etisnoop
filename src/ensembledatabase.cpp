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

#include "ensembledatabase.hpp"

namespace ensemble_database {

using namespace std;

component_t& service_t::get_component(uint32_t subchannel_id)
{
    for (auto& component : components) {
        if (component.subchId == subchannel_id) {
            return component;
        }
    }

    throw not_found("component referring to subchannel " +
            to_string(subchannel_id) + " not found");
}

component_t& service_t::get_or_create_component(uint32_t subchannel_id)
{
    for (auto& component : components) {
        if (component.subchId == subchannel_id) {
            return component;
        }
    }

    // not found
    component_t new_component;
    new_component.subchId = subchannel_id;
    components.push_back(new_component);
    return components.back();
}


service_t& ensemble_t::get_service(uint32_t service_id)
{
    for (auto& service : services) {
        if (service.id == service_id) {
            return service;
        }
    }

    throw not_found("Service " + to_string(service_id) + " not found");
}

service_t& ensemble_t::get_or_create_service(uint32_t service_id)
{
    for (auto& service : services) {
        if (service.id == service_id) {
            return service;
        }
    }

    // not found
    service_t new_service;
    new_service.id = service_id;
    services.push_back(new_service);
    return services.back();
}

subchannel_t& ensemble_t::get_subchannel(uint8_t subchannel_id)
{
    for (auto& subchannel : subchannels) {
        if (subchannel.id == subchannel_id) {
            return subchannel;
        }
    }

    throw not_found("Subchannel " + to_string(subchannel_id) + " not found");
}

subchannel_t& ensemble_t::get_or_create_subchannel(uint8_t subchannel_id)
{
    for (auto& subchannel : subchannels) {
        if (subchannel.id == subchannel_id) {
            return subchannel;
        }
    }

    // not found
    subchannel_t new_subchannel;
    new_subchannel.id = subchannel_id;
    subchannels.push_back(new_subchannel);
    return subchannels.back();
}

}
