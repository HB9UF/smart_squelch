#ifndef _ADC_H
#define _ADC_H

#define SAMPLE_BUF_SIZE 1024

extern adcsample_t *first_sample, *last_sample;
extern binary_semaphore_t wait_for_sample;

void adc_init(void);

#endif
