#ifdef __cplusplus
extern "C"
{
#endif

#include "verimake.h"
#include "debug.h"

unsigned long verimake_pin_D0_D13[14] = {
    9, 10, 16, 17, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28};

unsigned long verimake_pin_A0_A5[6] = {
    0, 1, 2, 3, 4, 5};

unsigned long verimake_pin_select(unsigned long ulPin)
{
    if (ulPin < 14)
    {
        return verimake_pin_D0_D13[ulPin];
    }
    else if (ulPin >= 192 && ulPin < 198)
    {
        return verimake_pin_A0_A5[ulPin - 192];
    }
    return ulPin;
}

#ifdef __cplusplus
}

#endif