/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "adc.h"
#include "fft.h"
#include "ti_msp_dl_config.h"

// volatile uint16_t gADCResult[ADC_SIZE];
volatile float32_t magnitude[ADC_SIZE / 2 + 1];
volatile bool gCheckADC = false;

volatile uint16_t gAdcResult[ADC_SIZE];
volatile uint16_t gAdcResult1[ADC_SIZE];
volatile uint16_t gAdcResult2[ADC_SIZE];
volatile float amp[4];//归一化幅值
volatile double Fre = 0;
volatile double Fre1 = 0;
volatile uint16_t FUNDAMENTAL_BIN=0;
volatile double SAMPLING_RATE=0;
volatile float THD = 0;

int main(void)
{
    SYSCFG_DL_init();

    /* Configure DMA source, destination and size */
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) DL_ADC12_getFIFOAddress(ADC12_0_INST));

    // DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) &gADCResult[0]);

    // DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    /* Setup interrupts on device */
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);

    // my_fft_init();

    
    // set_ADC_fre(1000.0);
    // my_ADC();
    while (1)
    {
        // my_fft((uint16_t *)gADCResult, (float32_t *)magnitude);
        //粗测
        SAMPLING_RATE = 500000.0;
        set_ADC_fre(SAMPLING_RATE);
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&gAdcResult[0]);
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
        my_ADC();
        Fre = processFFT_find();//得到第一次粗测的频率
        DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);

        //第一次精测
        SAMPLING_RATE = 8*Fre;
        set_ADC_fre(SAMPLING_RATE);
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&gAdcResult1[0]);
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
        my_ADC();
        FUNDAMENTAL_BIN = (int)(ADC_SIZE/8);//此时第一次粗测的基波频率对应此次采样频率大致的点数就是1024/8，后面再精测
        Fre1 = processFFT_find2(FUNDAMENTAL_BIN);//第二次精测的基波频率
        DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);

        //第二次精测
        SAMPLING_RATE = Fre1 * 16;
        set_ADC_fre(SAMPLING_RATE);
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&gAdcResult2[0]);
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
        my_ADC();
        FUNDAMENTAL_BIN = (int)(ADC_SIZE/16);
        THD = processFFT();//计算出失真度
        THD = THD * 100;//换为百分比
        DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
    }
}