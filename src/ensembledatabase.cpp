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

#include <locale>
#include <codecvt>
#include <sstream>
#include "ensembledatabase.hpp"
#include "charset.hpp"

namespace ensemble_database {

using namespace std;

static string ucs2toutf8(const uint8_t *ucs2, size_t len_bytes)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> ucsconv;

    wstring ucs2label;

    for (size_t i = 0; i < len_bytes-1; i+=2) {
        ucs2label += (wchar_t)(ucs2[i] * 256 + ucs2[i+1]);
    }

    // Can throw range_error
    return ucsconv.to_bytes(ucs2label);
}

std::string label_t::label() const
{
    switch (charset) {
        case charset_e::COMPLETE_EBU_LATIN:
            return convert_ebu_to_utf8(string(label_bytes.begin(), label_bytes.end()));
        case charset_e::UTF8:
            return string(label_bytes.begin(), label_bytes.end());
        case charset_e::UCS2:
            try {
                return ucs2toutf8(label_bytes.data(), label_bytes.size());
            }
            catch (const range_error&) {
                return "";
            }
        case charset_e::UNDEFINED:
            throw logic_error("charset undefined");
    }
    throw logic_error("invalid charset " + to_string((int)charset));
}

std::string label_t::shortlabel() const
{
    string shortlabel;
    for (size_t i = 0; i < label_bytes.size(); ++i) {
        if (shortlabel_flag & 0x8000 >> i) {
            shortlabel += static_cast<char>(label_bytes[i]);
        }
    }

    return shortlabel;
}

string label_t::assemble() const
{
    vector<uint8_t> segments_cat;
    for (size_t i = 0; i < segment_count; i++) {
        if (segments.count(i) == 0) {
            return "";
        }
        else {
            const auto& s = segments.at(i);
            copy(s.begin(), s.end(), back_inserter(segments_cat));
        }
    }

    switch (extended_label_charset) {
        case charset_e::COMPLETE_EBU_LATIN:
            // FIG2 doesn't allow EBU, use FIG1 for those
            return "";
        case charset_e::UTF8:
            return string(segments_cat.begin(), segments_cat.end());
        case charset_e::UCS2:
            try {
                return ucs2toutf8(segments_cat.data(), segments_cat.size());
            }
            catch (const range_error&) {
                return "";
            }
        case charset_e::UNDEFINED:
            return "";
    }
    throw logic_error("invalid extended label charset " + to_string((int)extended_label_charset));
}

string label_t::assembly_state() const
{
    stringstream ss;
    ss << "[";
    for (const auto& s : segments) {
        ss << s.first << ",";
    }

    ss << "count=" << segment_count << ",";
    ss << "charset=";
    switch (extended_label_charset) {
        case charset_e::COMPLETE_EBU_LATIN:
            throw logic_error("invalid extended label LATIN charset");
        case charset_e::UTF8:
            ss << "UTF8";
            break;
        case charset_e::UCS2:
            ss << "UCS2";
            break;
        case charset_e::UNDEFINED:
            ss << "UNDEFINED";
            break;
    }
    ss << "]";

    return ss.str();
}

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
