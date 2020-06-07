/* HB9UF smart squelch in a nutshell:
 *
 * The microcontroller samples ADC channel 0 at a rate of 50 kHz. This pin is
 * expected to be fed with a buffered (i.e. low impedance) discriminator tap
 * output from which audio frequencies are removed. Typiclly, this is achieved
 * with a HPF where frequencies below ~3 kHz are attenuated strongly.
 *
 * The samples are stored in a buffer. When the buffer is half full, the energy
 * of the samples is calculated and accumulated. Once every 8 iterations
 * (i.e. roughly 10 times per second), the accumulator is used for squelching.
 * Finally, the accumulator is reset for the next cycle. A backlog of accmulator
 * values is kept in a circular buffer.
 *
 * Squelch logic is as follows (accumulator =a, "mean" in the code):
 * 1. Squelch is opened if a < WEAK_THRESHOLD
 * 2. Squelch is immediately closed if a > WEAK_THRESHOLD and the backlog cotains
 *    values < STRONG_THRESHOLD without exception.
 * 3. Squelch remains open as long as the backlog contains at least one value
 *    below WEAK_THRESHOLD (grace tail).
 *
 *  This methodology eliminates the squelch crash for quieting stations, while
 *  weak stations, fading stations and statios with picket-fencing are not
 *  artifically impaired by our own squelching action.
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "io.h"
#include "backlog.h"
#include "adc_gpt.h"
#include "uart.h"

// FIXME: make this configurable
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
    bool squelch_armed = false;
    uint32_t mean = 0;
    uint8_t counter = 0;
    char squelch_flag = ' ';
    while (true)
    {
        chBSemWait(&wait_for_sample);
        // palClearPad(GPIOA, 4); // Use this to check how much idle time we have per cycle
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
                    // Immediately close squelch
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
                    // Close squelch afer grace tail
                    io_close_squelch();
                    squelch_open = false;
                    squelch_flag = ' ';
                }
            }
            else if(squelch_open && mean < WEAK_THRESHOLD)
            {
                // Set correct flag (otherwise the flag remains '?')
                squelch_flag = '*';
            }
            else if(!squelch_open && !squelch_armed && mean < WEAK_THRESHOLD)
            {
                // Arm squelch
                squelch_armed = true;
                squelch_flag = 'A';
            }
            else if(!squelch_open && squelch_armed && mean > WEAK_THRESHOLD)
            {
                // Disarm squelch
                squelch_armed = false;
                squelch_flag = ' ';
            }
            else if(!squelch_open && squelch_armed && mean < WEAK_THRESHOLD)
            {
                // Open squelch
                io_open_squelch();
                circular_buf_reset(&backlog, 0);
                squelch_open = true;
                opening_squelch = true;
                squelch_armed = false;
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
        // palSetPad(GPIOA, 4); // See comment above

    }
}
