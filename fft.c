#include "fft.h"

extern volatile bool gCheckADC;
// arm_rfft_fast_instance_f32 rfft_inst;

// void my_fft_init() { arm_rfft_fast_init_f32(&rfft_inst, ADC_SIZE); }

// void my_fft(uint16_t *gADCResult, float32_t *magnitude) {
//   while (gCheckADC == false)
//     ;
//   float32_t fft_input[ADC_SIZE];
//   float32_t gFFTResult[ADC_SIZE];
//   for (int i = 0; i < ADC_SIZE; i++) {
//     fft_input[i] = gADCResult[i];
//   }
//   arm_rfft_fast_f32(&rfft_inst, fft_input, gFFTResult, 0);
//   arm_cmplx_mag_f32(gFFTResult, magnitude, ADC_SIZE / 2 + 1);
//   gCheckADC = false;
// }



//fft操作：输入的real[]和imag[]替换为FFT后的频谱序列
void fft(double real[], double imag[], int n)
{
    int i, j, k, m;
    int stages = 0;
    int n1, n2;
    float c, s, t1, t2;

    // 计算级数：stages = log2(n)
    for (m = n; m > 1; m >>= 1)
        stages++;

    // 位反转排序
    j = 0;
    for (i = 0; i < n - 1; i++) {
        if (i < j) {
            // 交换实部
            t1 = real[i];
            real[i] = real[j];
            real[j] = t1;
            // 交换虚部
            t1 = imag[i];
            imag[i] = imag[j];
            imag[j] = t1;
        }
        k = n >> 1;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    // FFT蝶形运算
    n2 = 1;
    for (i = 0; i < stages; i++) {
        n1 = n2;
        n2 <<= 1;  // n2 = 2^(i+1)
        for (j = 0; j < n1; j++) {
            // 计算旋转因子
            c = cos(-2 * M_PI * j / n2);
            s = sin(-2 * M_PI * j / n2);
            for (k = j; k < n; k += n2) {
                t1 = c * real[k + n1] - s * imag[k + n1];
                t2 = s * real[k + n1] + c * imag[k + n1];
                real[k + n1] = real[k] - t1;
                imag[k + n1] = imag[k] - t2;
                real[k] += t1;
                imag[k] += t2;
            }
        }
    }
}

//找到基波对应的BIN（索引）
int findFundamentalBin(const double magnitude[], int numBins)
{
    int fundamentalBin = 2;  // 忽略直流（bin0）
    double maxVal = magnitude[2];
    for (int i = 2; i < numBins; i++) {
        if (magnitude[i] > maxVal) {
            maxVal = magnitude[i];
            fundamentalBin = i;
        }
    }
    return fundamentalBin;
}

//在第一次粗测，第一次精测所得基波频率对应的索引附近重新寻找最大幅值位置
int findBin(const double magnitude[], int fundamentalBins)
{
    int Bin;
    if(fundamentalBins <= 4){
        Bin = 1;
    }
    else{
        Bin = fundamentalBins-4;
    }
    float maxVal = magnitude[Bin];
    for (int i = Bin + 1; i <= fundamentalBins+4; i++) 
    {
        if (magnitude[i] > maxVal) 
        {
            maxVal = magnitude[i];
            Bin = i;
        }
    }
    return Bin;
}

double processFFT_find()
{
    while(gCheckADC==false);
    int i;
    double fftReal[ADC_SIZE];
    double fftImag[ADC_SIZE];
    double magnitude[ADC_SIZE/2];  // 仅计算前半部分的幅值

    //加汉宁窗：
    for (i = 0; i < ADC_SIZE; i++) {
        float window = 0.5f * (1.0f - cos(2.0f * M_PI * i / (ADC_SIZE - 1)));
        fftReal[i] = (float)gAdcResult[i] * window;
        fftImag[i] = 0.0;
    }

    // 执行FFT计算
    fft(fftReal, fftImag, ADC_SIZE);

    // 计算幅值谱：只计算0~(N/2-1)频率分量
    for (i = 0; i < ADC_SIZE/2; i++) {
        magnitude[i] = sqrt(fftReal[i]*fftReal[i] + fftImag[i]*fftImag[i]);
    }

    // 自适应检测基波：扫描幅值谱找出最大峰值（跳过直流分量）
    int fundamentalBin = findFundamentalBin(magnitude, ADC_SIZE/2);
    
    // 使用二次插值细化峰值位置
    //double refindBin = quadraticInterpolatePeak(magnitude, ADC_SIZE/2, fundamentalBin);

    // 根据采样率和FFT点数，可以计算出基波频率（第一次粗测：采样频率为500000）
    double fundamentalFreq = (SAMPLING_RATE) * fundamentalBin / ADC_SIZE;
    gCheckADC=false;
    return(fundamentalFreq) ; 
}

//第二次FFT精测频率，以8*fre为采样频率重新进行ADC采样
double processFFT_find2(int fundamentalBin)
{
    while(gCheckADC==false);
    int i;
    double fftReal[ADC_SIZE];
    double fftImag[ADC_SIZE];
    double magnitude[ADC_SIZE/2];  // 仅计算前半部分的幅值

    //加汉宁窗：
    for (i = 0; i < ADC_SIZE; i++) {
        float window = 0.5f * (1.0f - cos(2.0f * M_PI * i / (ADC_SIZE - 1)));
        fftReal[i] = (float)gAdcResult1[i] * window;
        fftImag[i] = 0.0;
    }

    // 执行FFT计算
    fft(fftReal, fftImag, ADC_SIZE);

    // 计算幅值谱：只计算0~(N/2-1)频率分量
    for (i = 0; i < ADC_SIZE/2; i++) {
        magnitude[i] = sqrt(fftReal[i]*fftReal[i] + fftImag[i]*fftImag[i]);
    }

    int actualFundamentalBin=findBin(magnitude,fundamentalBin);
    
    // 使用二次插值细化峰值位置
    //double refindBin = quadraticInterpolatePeak(magnitude, ADC_SIZE/2, actualFundamentalBin);

    // 根据采样率和FFT点数，可以计算出基波频率
    double fundamentalFreq = (SAMPLING_RATE) * actualFundamentalBin / ADC_SIZE;
    gCheckADC=false;
    return(fundamentalFreq) ; 
}


//执行THD计算
float computeTHD(const double magnitude[], int numBins, int fundamentalBin)
{
    int actualFundamentalBin=findBin(magnitude,fundamentalBin);
    //float fundamental = magnitude[actualFundamentalBin]+magnitude[actualFundamentalBin-1]+magnitude[actualFundamentalBin+1];
    // float fundamental = magnitude[actualFundamentalBin]+magnitude[actualFundamentalBin-1]+magnitude[actualFundamentalBin+1]+magnitude[actualFundamentalBin-2]+magnitude[actualFundamentalBin+2];
    float fundamental = magnitude[actualFundamentalBin];
    if (fundamental == 0.0f) {
        return 0.0f; // 避免除以0
    }

    float harmonicSumSq = 0.0f;
    int actualBin;
    // 从第2谐波开始累加（n=2,3,4...），只要n*fundamentalBin在幅值谱范围内
    for (int n = 2; (n * actualFundamentalBin < numBins/2)&&(n <= 7); n++) {
        actualBin = findBin(magnitude, n * actualFundamentalBin);
        //harmonicSumSq += pow((magnitude[actualBin]+magnitude[actualBin-1]+magnitude[actualBin+1]) ,2);
        // harmonicSumSq += pow((magnitude[actualBin]+magnitude[actualBin-1]+magnitude[actualBin+1]+magnitude[actualBin-2]+magnitude[actualBin+2]) ,2);
        harmonicSumSq += pow(magnitude[actualBin],2);
        if(n<6){
        //amp[n-2] = (magnitude[actualBin]+magnitude[actualBin-1]+magnitude[actualBin+1])/fundamental;
        // amp[n-2] = (magnitude[actualBin]+magnitude[actualBin-1]+magnitude[actualBin+1]+magnitude[actualBin-2]+magnitude[actualBin+2])/fundamental;
        amp[n-2] = magnitude[actualBin]/fundamental;//归一化幅值
        }
    }
    // THD计算公式：THD = RMS(谐波) / 基波
    float thd = sqrt(harmonicSumSq) / fundamental;
    return thd;
}

//第三次FFT得出THD
float processFFT(void)
{
    while(gCheckADC==false);
    int i;
    double fftReal[ADC_SIZE];
    double fftImag[ADC_SIZE];
    double magnitude[ADC_SIZE/2];  // 仅计算前半部分的幅值

    //使用平顶窗：（幅值精度很高）
    for (i = 0; i < ADC_SIZE; i++) {
        float phase = (float)i / (ADC_SIZE - 1);
        float window = (a0
                - a1 * cosf(2.0f * M_PI * phase)
                + a2 * cosf(4.0f * M_PI * phase)
                - a3 * cosf(6.0f * M_PI * phase)
                + a4 * cosf(8.0f * M_PI * phase))/4.634;
        fftReal[i] = (float)gAdcResult2[i] * window;
        fftImag[i] = 0.0;

    }


    // 执行FFT计算
    fft(fftReal, fftImag, ADC_SIZE);

    // 计算幅值谱：只计算0~(N/2-1)频率分量
    for (i = 0; i < ADC_SIZE/2; i++) {
        magnitude[i] = sqrt(fftReal[i]*fftReal[i] + fftImag[i]*fftImag[i]);
    }

    // 计算总谐波失真度（THD）
    float thd1 = computeTHD(magnitude, ADC_SIZE, FUNDAMENTAL_BIN);
    gCheckADC=false;
    return thd1;
}

