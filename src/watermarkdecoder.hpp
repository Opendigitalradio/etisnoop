/*
    Copyright (C) 2014 CSP Innovazione nelle ICT s.c.a r.l. (http://www.csp.it/)
    Copyright (C) 2022 Matthias P. Braendli (http://www.opendigitalradio.org)
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
#include <vector>
#include <sstream>
#include "utils.hpp"

class WatermarkDecoder
{
    public:
        WatermarkDecoder();

        // The order of FIG0_1 subchannels represents the bits
        void push_fig0_1_bit(bool bit);

        // The ConfInd of FIG 0/10 contains the CRC-DABMUX and ODR-DabMux watermark
        void push_confind_bit(bool confind);

        std::string calculate_watermark();

    private:
        const WatermarkDecoder& operator=(const WatermarkDecoder&) = delete;
        WatermarkDecoder(const WatermarkDecoder&) = delete;

        std::vector<bool> m_confind_bits;
        std::vector<bool> m_fig0_1_bits;
};

