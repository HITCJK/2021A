#include "adc.h"

extern volatile bool gCheckADC;

void ADC12_0_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC12_0_INST))
    {
    case DL_ADC12_IIDX_DMA_DONE:
        DL_TimerA_stopCounter(TIMER_0_INST);
        // DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
        gCheckADC = true;
        // DL_TimerA_startCounter(TIMER_0_INST);
        break;
    default:
        break;
    }
}

void my_ADC()
{
    DL_TimerA_startCounter(TIMER_0_INST);
}

void set_ADC_fre(double fre)
{
    DL_TimerA_TimerConfig ClockConfig = {
        .period = 32000000 / fre,
        .timerMode = DL_TIMER_TIMER_MODE_PERIODIC,
        .startTimer = DL_TIMER_STOP,
    };
    DL_TimerA_initTimerMode(TIMER_0_INST, &ClockConfig);
}