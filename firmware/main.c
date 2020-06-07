#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "string.h"

#define BACKLOG_SIZE 100

//#define io_set_led()   palSetPad(GPIOA, 4)
//#define io_clear_led() palClearPad(GPIOA, 4)

static adcsample_t sample[1] = {0};
volatile adcsample_t current_sample = 0;
static adcsample_t sample_backlog[BACKLOG_SIZE] = {0};
volatile int32_t sum = 0;
volatile uint8_t valid_sample_counter = 0;

binary_semaphore_t wait_for_sample;

typedef struct config_t
{
    uint16_t debug_period_ms; // How often to print statistics. Every x ms, 0 means never
    adcsample_t threshold;    // Sample 
} config_t;

typedef struct stats_t
{
    adcsample_t min;
    adcsample_t max;
    adcsample_t min2;
    uint16_t n_cos;
} stats_t;

static config_t config =
{
    debug_period_ms :  100,
    threshold:         200,
};

static stats_t stats = 
{
    min: 0xffff,
    min2: 0xffff,
    max: 0,
    n_cos: 0,
};

static void adc_callback(ADCDriver *adcp, adcsample_t *buffer, size_t n)
{
    static uint8_t pointer = 0;
    static uint8_t counter = 0;
    /* We use every sample, but only every 5th for the history. Thus, the 
     * sampling rate is 1 kHz but the backlog is of a full second, given a
     * backlog buffer of 200 entries. (see #define BACKLOG_SIZE)
     */
    current_sample = *buffer;

    if(5 == counter++)
    {
        counter = 0;
        sample_backlog[pointer] = *buffer;
        sum += sample_backlog[pointer];
        sum -= sample_backlog[(pointer+1) % BACKLOG_SIZE];
        if(valid_sample_counter < BACKLOG_SIZE) valid_sample_counter++;
        //chprintf((BaseSequentialStream*)&SD1, "%d %d %d\r\n", sample_backlog[pointer], sample_backlog[(pointer+1) % 200], sum/200);
        pointer = (pointer + 1) % BACKLOG_SIZE;
    }
    chBSemSignalI(&wait_for_sample);
}

const ADCConversionGroup adcgrpcfg = {
    circular:     TRUE, 
    num_channels: 1, 
    end_cb:       adc_callback,
    error_cb:     NULL,
    cfgr1:        ADC_CFGR1_EXTEN_0 | ADC_CFGR1_RES_12BIT,
    tr:           ADC_TR(0, 0),
    smpr:         ADC_SMPR_SMP_1P5,
    chselr:       ADC_CHSELR_CHSEL0
};


static const GPTConfig gptcfg = {
    frequency:    10000U,
    callback:     NULL,
    cr2:          TIM_CR2_MMS_1,  /* MMS = 010 = TRGO on Update Event.        */
    dier:         0U
};


static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg)
{
    (void)arg;
    chRegSetThreadName("console");
    while (true)
    {
        msg_t key = chSequentialStreamGet((BaseSequentialStream*)&SD1);
        stats.min = 0xffff;
        stats.max = 0;
        stats.min2 = 0xffff;
        stats.n_cos = 0;
    }
}

int main(void)
{
    bool cos = false;
    halInit();
    chSysInit();

    palSetPad(GPIOA, GPIOA_LED_GREEN); // let it pass
    palSetPadMode(GPIOA,  4, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOA,  5, PAL_MODE_OUTPUT_OPENDRAIN);

    palSetPadMode(GPIOF,  0, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOF,  1, PAL_MODE_OUTPUT_PUSHPULL);
    //palSetPadMode(GPIOF,  1, PAL_MODE_OUTPUT_OPENDRAIN | PAL_MODE_INPUT_PULLUP);
    palSetPad(GPIOF, 1);

    palSetPadMode(GPIOA,  0, PAL_MODE_INPUT_ANALOG);
    adcStart(&ADCD1, NULL);

    palSetPadMode(GPIOA,  9, PAL_MODE_ALTERNATE(1));
    palSetPadMode(GPIOA, 10, PAL_MODE_ALTERNATE(1));
    sdStart(&SD1, NULL);

    chBSemObjectInit(&wait_for_sample, TRUE);


    chThdSleepMilliseconds(1000);
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

    adcStartConversion(&ADCD1, &adcgrpcfg, sample, 1);
    gptStart(&GPTD1, &gptcfg);
    GPTD1.tim->CR2 &= ~TIM_CR2_MMS;
    GPTD1.tim->CR2 |= TIM_CR2_MMS_1;
    gptStartContinuous(&GPTD1, 10);

    uint16_t debug_print_counter = 0;
    adcsample_t last_sample = 0;
    char lag = ' ';

    while (true)
    {
        chBSemWait(&wait_for_sample);
        //palClearPad(GPIOF, 0);

        if(current_sample < stats.min) { stats.min = current_sample; stats.min2 = last_sample;}
        if(current_sample > stats.max) stats.max = current_sample;

        if(config.debug_period_ms && ++debug_print_counter == config.debug_period_ms)
        {
            debug_print_counter = 0;
            chprintf((BaseSequentialStream*)&SD1, "x=%d x̄=%d, min=%d, max=%d, n=%d, min2=%d\r\n", current_sample, sum/BACKLOG_SIZE, stats.min, stats.max, stats.n_cos, stats.min2);
        }

        if(cos && current_sample > 2*config.threshold)
        {
            if(last_sample < 50 || sum/BACKLOG_SIZE <= 20 || sum/BACKLOG_SIZE > 2*config.threshold)
            //if(sum/valid_sample_counter < 5 || sum/valid_sample_counter < config.threshold)
            {
                palSetPad(GPIOF, 1);
                chprintf((BaseSequentialStream*)&SD1, "%c   %d\r\n", lag, sum/BACKLOG_SIZE);
                cos = false;
            }
            else
            {
                lag = '+';
            }
        }
        if(!cos && current_sample < config.threshold && last_sample < config.threshold)
        {
            palClearPad(GPIOF, 1);
            cos = true;
            lag = ' ';
            valid_sample_counter = 1;
            chprintf((BaseSequentialStream*)&SD1, "COS %d\r\n", sum/BACKLOG_SIZE);
            stats.n_cos++;
        }

        last_sample = current_sample;
        //palSetPad(GPIOF, 0);
    }
}
