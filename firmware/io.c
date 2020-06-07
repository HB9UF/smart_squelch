#include "hal.h"

void io_init(void)
{
    //palSetPad(GPIOA, GPIOA_LED_GREEN); // we may have to activate this to disable the muting circuit
    palSetPadMode(GPIOA,  4, PAL_MODE_OUTPUT_PUSHPULL); // This is used to keep track of how idle we are
    palSetPadMode(GPIOF,  1, PAL_MODE_OUTPUT_PUSHPULL); // This is used to signal squelch (active low)
}

void io_open_squelch(void)
{
    palClearPad(GPIOF, 1);
}

void io_close_squelch(void)
{
    palSetPad(GPIOF, 1);
}
