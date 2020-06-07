#include "hal.h"
#include "adc_gpt.h"

adcsample_t *first_sample, *last_sample;
binary_semaphore_t wait_for_sample;

static adcsample_t sample_buffer[SAMPLE_BUF_SIZE] = {0};

static void adc_callback(ADCDriver *adcp, adcsample_t *buffer, size_t n)
{
    (void)*adcp; // This is just to get rid of the compiler warning that we never use adcp
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

void adc_init(void)
{
    chBSemObjectInit(&wait_for_sample, TRUE);
    palSetPadMode(GPIOA,  0, PAL_MODE_INPUT_ANALOG);
    adcStart(&ADCD1, NULL);

    adcStartConversion(&ADCD1, &adcgrpcfg, sample_buffer, SAMPLE_BUF_SIZE);
    gptStart(&GPTD1, &gptcfg);
    gptStartContinuous(&GPTD1, 20);
}
