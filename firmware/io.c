#include "hal.h"

void io_init(void)
{
    //palSetPad(GPIOA, GPIOA_LED_GREEN); // FIXME
    palSetPadMode(GPIOA,  4, PAL_MODE_OUTPUT_PUSHPULL); // FIXME
    palSetPadMode(GPIOF,  1, PAL_MODE_OUTPUT_PUSHPULL);
}

void io_open_squelch(void)
{
    palClearPad(GPIOF, 1);
}

void io_close_squelch(void)
{
    palSetPad(GPIOF, 1);
}
