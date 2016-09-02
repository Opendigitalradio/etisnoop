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

#pragma once
#include <vector>
#include <sstream>
#include "utils.hpp"

class WatermarkDecoder
{
    public:
        WatermarkDecoder() {}

        void push_confind_bit(bool confind)
        {
            // The ConfInd of FIG 0/10 contains the CRC-DABMUX and ODR-DabMux watermark
            m_confind_bits.push_back(confind);
        }

        std::string calculate_watermark()
        {
            // First try to find the 0x55 0x55 sync in the waternark data
            size_t bit_ix;
            int alternance_count = 0;
            bool last_bit = 1;
            for (bit_ix = 0; bit_ix < m_confind_bits.size(); bit_ix++) {
                if (alternance_count == 16) {
                    break;
                }
                else {
                    if (last_bit != m_confind_bits[bit_ix]) {
                        last_bit = m_confind_bits[bit_ix];
                        alternance_count++;
                    }
                    else {
                        alternance_count = 0;
                        last_bit = 1;
                    }
                }

            }

            printf("Found SYNC at offset %zu out of %zu\n", bit_ix - alternance_count, m_confind_bits.size());

            std::stringstream watermark_ss;

            uint8_t b = 0;
            size_t i = 0;
            while (bit_ix < m_confind_bits.size()) {

                b |= m_confind_bits[bit_ix] << (7 - i);

                if (i == 7) {
                    watermark_ss << (char)b;

                    b = 0;
                    i = 0;
                }
                else {
                    i++;
                }

                bit_ix += 2;
            }

            return watermark_ss.str();
        }

    private:
        const WatermarkDecoder& operator=(const WatermarkDecoder&) = delete;
        WatermarkDecoder(const WatermarkDecoder&) = delete;

        std::vector<bool> m_confind_bits;
};

