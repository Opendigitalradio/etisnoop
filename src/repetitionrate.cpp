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

#include "repetitionrate.hpp"
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <limits>

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
};


static map<FIGTypeExt, FIGRateInfo> fig_rates;

static int current_frame_number = 0;
static int current_fib = 0;

void rate_announce_fig(int figtype, int figextension, bool complete)
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
}

// Calculate the minimal, maximum and average repetition rate (FIGs per
// second).
void rate_min_max_avg(
        const std::vector<int>& fig_positions,
        double* min,
        double* max,
        double* avg,
        bool per_second)
{
    double avg_interval =
        (double)(fig_positions.back() - fig_positions.front()) /
        (double)(fig_positions.size() - 1);

    int min_delta = std::numeric_limits<int>::max();
    int max_delta = 0;
    for (size_t i = 1; i < fig_positions.size(); i++) {
        const int delta = fig_positions[i] - fig_positions[i-1];

        // Min
        if (min_delta > delta) {
            min_delta = delta;
        }

        // Max
        if (max_delta < delta) {
            max_delta = delta;
        }
    }

    *avg = avg_interval;
    *min = min_delta;
    *max = max_delta;

    if (per_second) {
        *avg = 1.0 /
            (*avg * FRAME_DURATION);
        *min = 1.0 / (*min * FRAME_DURATION);
        *max = 1.0 / (*min * FRAME_DURATION);
    }

}

void rate_display_header(bool per_second)
{
    if (per_second) {
        printf("FIG carousel analysis. Format:\n"
                "  min, average, max FIGs per second (total count) - \n"
                "  min, average, max complete FIGs per second (total count)\n");
    }
    else {
        printf("FIG carousel analysis. Format:\n"
                "  min, average, max frames per FIG (total count) - \n"
                "  min, average, max frames per complete FIGs (total count)\n");
    }
}

void rate_display_analysis(bool clear, bool per_second)
{
    for (auto& fig_rate : fig_rates) {
        auto& frames_present = fig_rate.second.frames_present;
        auto& frames_complete = fig_rate.second.frames_complete;

        double min = 0.0;
        double max = 0.0;
        double avg = 0.0;

        const size_t n_present = frames_present.size();
        const size_t n_complete = frames_complete.size();
        if (n_present >= 2) {

            rate_min_max_avg(frames_present, &min, &max, &avg, per_second);

            printf("FIG%2d/%2d %4.2f %4.2f %4.2f (%5zu)",
                    fig_rate.first.figtype, fig_rate.first.figextension,
                    min, avg, max,
                    n_present);

            if (n_complete >= 2) {
                rate_min_max_avg(frames_complete, &min, &max, &avg, per_second);

                printf(" - %4.2f %4.2f %4.2f (%5zu)",
                        min, avg, max,
                        n_present);
            }
            else {
                printf(" - None complete");
            }
        }
        else {
            printf("FIG%2d/%2d 0",
                    fig_rate.first.figtype, fig_rate.first.figextension);
        }

        printf(" - in FIB(s):");
        for (auto& fib : fig_rate.second.in_fib) {
            printf(" %d", fib);
        }
        printf("\n");

    }

    if (clear) {
        fig_rates.clear();
    }
}

void rate_new_fib(int fib)
{
    if (fib == 0) {
        current_frame_number++;
    }

    current_fib = fib;
}

