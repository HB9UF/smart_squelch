#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "io.h"
#include "backlog.h"
#include "adc_gpt.h"
#include "uart.h"

const uint32_t WEAK_THRESHOLD = 38000;
const uint32_t STRONG_THRESHOLD = 7000;

int main(void)
{
    halInit();
    chSysInit();
    io_init();
    uart_init();
    adc_init();

    circular_buf_t backlog;
    circular_buf_init(&backlog);

    bool squelch_open = false;
    uint32_t mean = 0;
    uint8_t counter = 0;
    char squelch_flag = ' ';
    while (true)
    {
        chBSemWait(&wait_for_sample);
        palClearPad(GPIOA, 4);
        uint32_t sum = 0;
        for(adcsample_t *s = first_sample; s < last_sample; s++)
        {
            int16_t tmp = (int16_t) *s;
            sum += (tmp-2047)*(tmp-2047);
        }
        mean += sum/8;
        if(++counter == 8)
        {
            // We hit this condition approx. 12 times per second
            mean /= 1000; // Divide by 1000 to get numbers that are easier to handle
            bool opening_squelch = false;
            if(squelch_open && mean > WEAK_THRESHOLD)
            {
                if(circular_buf_all_lower_than(&backlog, STRONG_THRESHOLD))
                {
                    io_close_squelch();
                    squelch_open = false;
                    squelch_flag = ' ';
                }
                else if(circular_buf_any_lower_than(&backlog, WEAK_THRESHOLD))
                {
                    // Don't close squelch just yet, enter grace tail instead.
                    squelch_flag = '?';
                }
                else
                {
                    io_close_squelch();
                    squelch_open = false;
                    squelch_flag = ' ';
                }
            }
            else if(!squelch_open && mean < WEAK_THRESHOLD)
            {
                io_open_squelch();
                circular_buf_reset(&backlog, 0);
                squelch_open = true;
                opening_squelch = true;
                squelch_flag = '*';
            }
            if(!opening_squelch)
            {
                // We only add this to the backlog if we are sure that PTT was asserted for the full duration
                // of the acquisition. Otherwise, the mean may be too low and trigger the grace tail.
                circular_buf_append(&backlog, mean);
            }
            chprintf((BaseSequentialStream*)&SD1, "%c %c %d\r\n", squelch_open ? '!' : ' ', squelch_flag, mean);
            mean = 0;
            counter = 0;
        }
        palSetPad(GPIOA, 4);

    }
}
