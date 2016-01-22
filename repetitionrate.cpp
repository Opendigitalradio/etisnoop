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
#include <unordered_set>

using namespace std;

const double frame_duration = 24e-3;

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
    unordered_set<int> in_fib;
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

void rate_display_analysis(bool clear)
{
    printf("FIG carousel analysis. Format:\n"
         "  average FIGs per second (total count, delta variance in frames)\n"
         "  average complete FIGs per second (total count, delta variance in frames)\n");
    for (auto& fig_rate : fig_rates) {
        auto& frames_present = fig_rate.second.frames_present;
        auto& frames_complete = fig_rate.second.frames_complete;

        const size_t n_present = frames_present.size();
        if (n_present) {
            double average_present_interval =
                (double)(frames_present.back() - frames_present.front()) /
                (double)(frames_present.size() - 1);

            double variance_of_delta_present = 0;
            for (size_t i = 1; i < frames_present.size(); i++) {
                double s =
                    (double)(frames_present[i] - frames_present[i-1]) -
                    average_present_interval;

                variance_of_delta_present += s * s;
            }
            variance_of_delta_present /= frames_present.size();

            const double n_present_per_second = 1 /
                (average_present_interval * frame_duration);

            printf("FIG%2d/%2d %2.2f (%6zu %2.2f)",
                    fig_rate.first.figtype, fig_rate.first.figextension,
                    n_present_per_second, n_present, variance_of_delta_present);
        }

        const size_t n_complete = frames_complete.size();
        if (n_complete) {

            double average_complete_interval =
                (double)(frames_complete.back() - frames_complete.front()) /
                (double)(frames_complete.size() - 1);


            double variance_of_delta_complete = 0;
            for (size_t i = 1; i < frames_complete.size(); i++) {
                double s =
                    (double)(frames_complete[i] - frames_complete[i-1]) -
                    average_complete_interval;

                variance_of_delta_complete += s * s;
            }
            variance_of_delta_complete /= frames_complete.size();

            const double n_complete_per_second = 1 /
                (average_complete_interval * frame_duration);

            printf(" %2.2f (%6zu %2.2f)\n",
                    n_complete_per_second, n_complete, variance_of_delta_complete);
        }
        else {
            printf("FIG%2d/%2d 0\n",
                    fig_rate.first.figtype, fig_rate.first.figextension);
        }
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

