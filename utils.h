#include <stdlib.h>
#include <stdint.h>

#ifndef __UTILS_H_
#define __UTILS_H_
static inline
void    setBit(uint8_t x [], uint8_t bit, int32_t pos)
{
    int16_t iByte;
    int16_t iBit;

    iByte   = pos / 8;
    iBit    = pos % 8;
    x[iByte] = (x[iByte] & (~(1 << (7 - iBit)))) |
        (bit << (7 - iBit));
}

static inline
void    setBits(uint8_t x[], uint32_t bits,
        int32_t startPosition, int32_t numBits)
{
    int32_t i;
    uint8_t bit;

    for (i = 0; i < numBits; i ++) {
        bit = bits & (1 << (numBits - i - 1)) ? 1 : 0;
        setBit(x, bit, startPosition + i);
    }
}

#endif

