#pragma once
#include <vector>
#include <cstdio>

struct FIG
{
    int type;
    int ext;
    int len;
};

class FIGalyser
{
    public:
        FIGalyser()
        {
            clear();
        }

        void set_fib(int fib)
        {
            m_fib = fib;
        }

        void push_back(int type, int ext, int len)
        {
            struct FIG fig = {
                .type = type,
                .ext  = ext,
                .len  = len };

            m_figs[m_fib].push_back(fig);
        }

        void analyse(int mid)
        {
            printf("FIC ");

            for (size_t fib = 0; fib < (mid==3?4:3); fib++) {
                int consumed = 7;
                int fic_size = 0;
                printf("[%1zu ", fib);

                for (size_t i = 0; i < m_figs[fib].size(); i++) {
                    FIG &f = m_figs[fib][i];
                    printf("%01d/%02d (%2d) ", f.type, f.ext, f.len);

                    consumed += 10;

                    fic_size += f.len;
                }

                printf(" ");

                int align = 60 - consumed;
                if (align > 0) {
                    while (align--) {
                        printf(" ");
                    }
                }

                printf("|");

                for (int i = 0; i < 15; i++) {
                    if (2*i < fic_size) {
                        printf("#");
                    }
                    else {
                        printf("-");
                    }
                }

                printf("| ]   ");

            }

            printf("\n");
        }

        void clear()
        {
            m_figs.clear();
            m_figs.resize(4);
        }

    private:
        int m_fib;
        std::vector<std::vector<FIG> > m_figs;
};


