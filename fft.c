#include "fft.h"

extern volatile bool gCheckADC;
arm_rfft_fast_instance_f32 rfft_inst;

void my_fft_init() { arm_rfft_fast_init_f32(&rfft_inst, ADC_SIZE); }

void my_fft(uint16_t *gADCResult, float32_t *magnitude) {
  while (gCheckADC == false)
    ;
  float32_t fft_input[ADC_SIZE];
  float32_t gFFTResult[ADC_SIZE];
  for (int i = 0; i < ADC_SIZE; i++) {
    fft_input[i] = gADCResult[i];
  }
  arm_rfft_fast_f32(&rfft_inst, fft_input, gFFTResult, 0);
  arm_cmplx_mag_f32(gFFTResult, magnitude, ADC_SIZE / 2 + 1);
  gCheckADC = true;
}