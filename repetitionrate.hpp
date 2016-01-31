/*
    Copyright (C) 2016 Matthias P. Braendli (http://www.opendigitalradio.org)

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
         Matthias P. Braendli <matthias@mpb.li>

*/

#pragma once
#include "utils.hpp"

/* Tell the repetition rate analyser that we have received a given FIG.
 * The complete flag should be set to true every time a complete
 * set of information for that FIG has been received
 */
void rate_announce_fig(int figtype, int figextension, bool complete);

/* Tell the repetition rate analyser that a new FIB starts.
 */
void rate_new_fib(int fib);

/* Print small header to explain rate display
 * per_second: if true, rates are calculated in FIGs per second.
 * If false, rate is given in frames per FIG
 */
void rate_display_header(bool per_second);

/* Print analysis.
 * optionally clear all statistics
 * per_second: if true, rates are calculated in FIGs per second.
 * If false, rate is given in frames per FIG
 */
void rate_display_analysis(bool clear, bool per_second);

