#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "io.h"
#include "backlog.h"

#define BUF_SIZE 1024
#define BACKLOG_BUF_SIZE 12

static adcsample_t sample_buffer[BUF_SIZE] = {0};
volatile adcsample_t *first_sample, *last_sample;
binary_semaphore_t wait_for_sample;

static void adc_callback(ADCDriver *adcp, adcsample_t *buffer, size_t n)
{
    first_sample = buffer;
    last_sample = buffer+n;
    chBSemSignalI(&wait_for_sample);
}

const ADCConversionGroup adcgrpcfg = {
    circular:     TRUE, 
    num_channels: 1, 
    end_cb:       adc_callback,
    error_cb:     NULL,
    cfgr1:        ADC_CFGR1_EXTEN_0 | ADC_CFGR1_RES_12BIT,
    tr:           ADC_TR(0, 0),
    smpr:         ADC_SMPR_SMP_7P5,
    chselr:       ADC_CHSELR_CHSEL0
};


static const GPTConfig gptcfg = {
    frequency:    1000000U,
    callback:     NULL,
    cr2:          TIM_CR2_MMS_1,  /* MMS = 010 = TRGO on Update Event.        */
    dier:         0U
};

const uint32_t WEAK_THRESHOLD = 38000;
const uint32_t STRONG_THRESHOLD = 7000;

int main(void)
{
    halInit();
    chSysInit();
    io_init();

    palSetPadMode(GPIOA,  0, PAL_MODE_INPUT_ANALOG);
    adcStart(&ADCD1, NULL);

    palSetPadMode(GPIOA,  9, PAL_MODE_ALTERNATE(1));
    palSetPadMode(GPIOA, 10, PAL_MODE_ALTERNATE(1));
    sdStart(&SD1, NULL);

    chBSemObjectInit(&wait_for_sample, TRUE);

    chThdSleepMilliseconds(1000);

    adcStartConversion(&ADCD1, &adcgrpcfg, sample_buffer, BUF_SIZE);
    gptStart(&GPTD1, &gptcfg);
    gptStartContinuous(&GPTD1, 20);

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
