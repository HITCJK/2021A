#include "arm_math.h"
#include "ti_msp_dl_config.h"

#define ADC_SIZE (1024)

void my_fft_init();
void my_fft(uint16_t *gADCResult, float32_t *magnitude);