#include "arm_math.h"
#include "ti_msp_dl_config.h"

#define ADC_SIZE (1024)

static const float a0 =  1.000000f;
static const float a1 =  1.930000f;
static const float a2 =  1.290000f;
static const float a3 =  0.388000f;
static const float a4 =  0.032200f;
extern volatile uint16_t gAdcResult[ADC_SIZE];
extern volatile uint16_t gAdcResult1[ADC_SIZE];
extern volatile uint16_t gAdcResult2[ADC_SIZE];
extern volatile float amp[4];//归一化幅值
extern volatile double Fre;
extern volatile double Fre1;
extern volatile uint16_t FUNDAMENTAL_BIN;
extern volatile double SAMPLING_RATE;
extern volatile float THD;

double processFFT_find();
double processFFT_find2(int fundamentalBin);
float processFFT(void);

// void my_fft_init();
// void my_fft(uint16_t *gADCResult, float32_t *magnitude);