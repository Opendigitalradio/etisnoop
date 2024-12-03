/*
    Copyright (C) 2024 Matthias P. Braendli (http://www.opendigitalradio.org)

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

#include "repetitionrate.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <set>

using namespace std;

const double FRAME_DURATION = 24e-3;

struct FIGTypeExt {
    int figtype;
    int figextension;

    bool operator<(const FIGTypeExt& other) const {
        if (this->figtype == other.figtype) {
            return this->figextension < other.figextension;
        }
        else {
            return this->figtype < other.figtype ;
        }
    }
};

struct FIGRateInfo {
    // List of frame numbers in which the FIG is present
    vector<int> frames_present;

    // List of frame numbers in which a complete DB for that FIG has been sent
    vector<int> frames_complete;

    // Which FIBs this FIG was seen in
    set<int> in_fib;

    vector<uint8_t> lengths;
};


static map<FIGTypeExt, FIGRateInfo> fig_rates;

static int current_frame_number = 0;
static int current_fib = 0;

void rate_announce_fig(int figtype, int figextension, bool complete, uint8_t figlen)
{
    FIGTypeExt f = {.figtype = figtype, .figextension = figextension};

    if (fig_rates.count(f) == 0) {
        FIGRateInfo rate;
        fig_rates[f] = rate;
    }

    FIGRateInfo& rate = fig_rates[f];

    rate.frames_present.push_back(current_frame_number);
    if (complete) {
        rate.frames_complete.push_back(current_frame_number);
    }
    rate.in_fib.insert(current_fib);
    rate.lengths.push_back(figlen);
}

static double rate_avg(
        const std::vector<int>& fig_positions,
        bool per_second)
{
    // Average interval is 1/N \sum_{i} pos_{i+1} - pos_{i}
    // but this sums out all the intermediate pos_{i}, leaving us
    // with 1/N (pos_{last} - pos_{first})
    //
    // N = size - 1 because we sum intervals, not points in time,
    // and there's one less interval than there are points.

    double avg =
        (double)(fig_positions.back() - fig_positions.front()) /
        (double)(fig_positions.size() - 1);
    if (per_second) {
        avg = 1.0 / (avg * FRAME_DURATION);
    }
    return avg;
}

static double length_avg(const std::vector<uint8_t>& lengths)
{
    double avg = 0.0;
    for (const uint8_t l : lengths) {
        avg += l;
    }
    return avg / (double)lengths.size();
}

static string length_histogram(const std::vector<uint8_t>& lengths)
{
    const array<const char*, 7> hist_chars({"▁", "▂", "▃", "▄", "▅", "▆", "▇"});

    // FIB length is 30
    vector<size_t> histogram(30);
    for (const uint8_t l : lengths) {
        histogram.at(l)++;
    }

    const double max_hist = *std::max_element(histogram.cbegin(), histogram.cend());

    stringstream ss;
    ss << "[";
    for (const size_t h : histogram) {
        uint8_t char_ix = floor((double)h / (max_hist+1) * hist_chars.size());
        ss << hist_chars.at(char_ix);
    }
    ss << "]";

    return ss.str();
}


void rate_display_analysis(bool per_second)
{

#define GREPPABLE_PREFIX "CAROUSEL "

    if (per_second) {
        printf(GREPPABLE_PREFIX
        "FIG T/EXT  AVG  (COUNT) -   AVG  (COUNT) -  LEN - LENGTH HISTOGRAM               IN FIB(S)\n");
    }

    for (auto& fig_rate : fig_rates) {
        auto& frames_present = fig_rate.second.frames_present;
        auto& frames_complete = fig_rate.second.frames_complete;

        const size_t n_present = frames_present.size();
        const size_t n_complete = frames_complete.size();
        printf(GREPPABLE_PREFIX);

        if (n_present >= 2) {
            double avg = rate_avg(frames_present, per_second);

            printf("FIG%2d/%2d %6.2f (%5zu)",
                    fig_rate.first.figtype, fig_rate.first.figextension,
                    avg,
                    n_present);

            if (n_complete >= 2) {
                double avg = rate_avg(frames_complete, per_second);

                printf(" - %6.2f (%5zu)", avg, n_complete);
            }
            else {
                printf(" - None complete ");
            }
        }
        else {
            printf("FIG%2d/%2d ",
                    fig_rate.first.figtype, fig_rate.first.figextension);
        }

        printf(" - %4.1f %s - ",
                length_avg(fig_rate.second.lengths),
                length_histogram(fig_rate.second.lengths).c_str());

        for (auto& fib : fig_rate.second.in_fib) {
            printf(" %d", fib);
        }
        printf("\n");

    }
}

void rate_new_fib(int fib)
{
    if (fib == 0) {
        current_frame_number++;
    }

    current_fib = fib;
}

