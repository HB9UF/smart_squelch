#include "hal.h"
#include "uart.h"

void uart_init(void)
{
    palSetPadMode(GPIOA,  9, PAL_MODE_ALTERNATE(1));
    palSetPadMode(GPIOA, 10, PAL_MODE_ALTERNATE(1));
    sdStart(&SD1, NULL);
}
