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

    Authors:
         Sergio Sagliocco <sergio.sagliocco@csp.it>
         Matthias P. Braendli <matthias@mpb.li>
                   / |  |-  ')|)  |-|_ _ (|,_   .|  _  ,_ \
         Data Path \(|(||_(|/_| (||_||(a)_||||(|||.(_()|||/

*/

#include "tables.hpp"
#include <string>
#include <vector>
#include <stdexcept>

static const std::vector<const char*> language_names = {
   "Unknown/not applicable",
   "Albanian",
   "Breton",
   "Catalan",
   "Croatian",
   "Welsh",
   "Czech",
   "Danish",
   "German",
   "English",
   "Spanish",
   "Esperanto",
   "Estonian",
   "Basque",
   "Faroese",
   "French",
   "Frisian",
   "Irish",
   "Gaelic",
   "Galician",
   "Icelandic",
   "Italian",
   "Lappish",
   "Latin",
   "Latvian",
   "Luxembourgian",
   "Lithuanian",
   "Hungarian",
   "Maltese",
   "Dutch",
   "Norwegian",
   "Occitan",
   "Polish",
   "Portuguese",
   "Romanian",
   "Romansh",
   "Serbian",
   "Slovak",
   "Slovene",
   "Finnish",
   "Swedish",
   "Turkish",
   "Flemish",
   "Walloon",
   "rfu",
   "rfu",
   "rfu",
   "rfu",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Reserved for national assignment",
   "Background sound/clean feed",
   "rfu",
   "rfu",
   "rfu",
   "rfu",
   "Zulu",
   "Vietnamese",
   "Uzbek",
   "Urdu",
   "Ukranian",
   "Thai",
   "Telugu",
   "Tatar",
   "Tamil",
   "Tadzhik",
   "Swahili",
   "Sranan Tongo",
   "Somali",
   "Sinhalese",
   "Shona",
   "Serbo-Croat",
   "Rusyn",
   "Russian",
   "Quechua",
   "Pushtu",
   "Punjabi",
   "Persian",
   "Papiamento",
   "Oriya",
   "Nepali",
   "Ndebele",
   "Marathi",
   "Moldavian",
   "Malaysian",
   "Malagasay",
   "Macedonian",
   "Laotian",
   "Korean",
   "Khmer",
   "Kazakh",
   "Kannada",
   "Japanese",
   "Indonesian",
   "Hindi",
   "Hebrew",
   "Hausa",
   "Gurani",
   "Gujurati",
   "Greek",
   "Georgian",
   "Fulani",
   "Dari",
   "Chuvash",
   "Chinese",
   "Burmese",
   "Bulgarian",
   "Bengali",
   "Belorussian",
   "Bambora",
   "Azerbaijani",
   "Assamese",
   "Armenian",
   "Arabic",
   "Amharic",
};

const char* get_language_name(size_t language_code)
{
    if (language_code < language_names.size())
    {
        return language_names[language_code];
    }

    throw std::runtime_error("Invalid language_code!");
}

// fig 0/18 0/19 announcement types (ETSI TS 101 756 V1.6.1 (2014-05) table 14 & 15)
const char *announcement_types_str[16] = {
    "Alarm",
    "Road Traffic flash",
    "Transport flash",
    "Warning/Service",
    "News flash",
    "Area weather flash",
    "Event announcement",
    "Special event",
    "Programme Information",
    "Sport report",
    "Financial report",
    "Reserved for future definition",
    "Reserved for future definition",
    "Reserved for future definition",
    "Reserved for future definition",
    "Reserved for future definition"
};

const char* get_announcement_type(size_t announcement_type)
{
    return announcement_types_str[announcement_type];
}


// fig 0/17 Programme type codes
#define INTERNATIONAL_TABLE_SIZE     2
#define PROGRAMME_TYPE_CODES_SIZE  32
const char *Programme_type_codes_str[INTERNATIONAL_TABLE_SIZE][PROGRAMME_TYPE_CODES_SIZE] = {
    {   // ETSI TS 101 756 V1.6.1 (2014-05) table 12
        "No programme type",        "News",
        "Current Affairs",          "Information",
        "Sport",                    "Education",
        "Drama",                    "Culture",
        "Science",                  "Varied",
        "Pop Music",                "Rock Music",
        "Easy Listening Music",     "Light Classical",
        "Serious Classical",        "Other Music",
        "Weather/meteorology",      "Finance/Business",
        "Children's programmes",    "Social Affairs",
        "Religion",                 "Phone In",
        "Travel",                   "Leisure",
        "Jazz Music",               "Country Music",
        "National Music",           "Oldies Music",
        "Folk Music",               "Documentary",
        "Not used",                 "Not used"
    },
    {   // ETSI TS 101 756 V1.6.1 (2014-05) table 13
        "No program type",  "News",
        "Information",      "Sports",
        "Talk",             "Rock",
        "Classic Rock",     "Adult Hits",
        "Soft Rock",        "Top 40",
        "Country",          "Oldies",
        "Soft",             "Nostalgia",
        "Jazz",             "Classical",
        "Rhythm and Blues", "Soft Rhythm and Blues",
        "Foreign Language", "Religious Music",
        "Religious Talk",   "Personality",
        "Public",           "College",
        "rfu",              "rfu",
        "rfu",              "rfu",
        "rfu",              "Weather",
        "Not used",         "Not used"
    }
};

const char* get_programme_type(size_t int_table_Id, size_t pty)
{
    if ((int_table_Id - 1) < INTERNATIONAL_TABLE_SIZE) {
       if (pty < PROGRAMME_TYPE_CODES_SIZE) {
           return Programme_type_codes_str[int_table_Id - 1][pty];
       }
       else {
           return "invalid programme type";
       }
    }
    else {
        return "unknown international table Id";
    }
}


const char *DSCTy_types_str[64] =  {
    // ETSI TS 101 756 V1.6.1 (2014-05) table 2
    "Unspecified data",                             "Traffic Message Channel (TMC)",
    "Emergency Warning System (EWS)",               "Interactive Text Transmission System (ITTS)",
    "Paging",                                       "Transparent Data Channel (TDC)",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "MPEG-2 Transport Stream, see ETSI TS 102 427", "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Rfu",
    "Rfu",                                          "Embedded IP packets",
    "Multimedia Object Transfer (MOT)",             "Proprietary service: no DSCTy signalled",
    "Not used",                                     "Not used"
};

const char* get_dscty_type(size_t dscty)
{
    return DSCTy_types_str[dscty];
}

// ETSI TS 102 367 V1.2.1 (2006-01) 5.4.1 Conditional Access Mode (CAMode)
// Used in FIG 0/3
const char *CAMode_str[8] = {
    "Sub-channel CA",   "Data Group CA",
    "MOT CA",           "proprietary CA",
    "reserved",         "reserved",
    "reserved",         "reserved"
};

const char* get_ca_mode(size_t ca_mode)
{
    return CAMode_str[ca_mode];
}

