#include "ADS1292_App.h"

volatile uint8_t drdy_flag = 0;
volatile uint8_t spi_rx_done = 0;
extern uint8_t ads1292_dma_buf[9];
uint8_t ads_frame_count=0; //已读帧数的数量
uint8_t spi_error_count=0;
ads1292_data_str ads1291_data;   



float ads_test_data[TEST_SAMPLES];

/* 调试统计变量（需在别处声明 extern，或移到头文件） */
int32_t raw_min = 0x7FFFFFFF;       // 最小原始值
int32_t raw_max = (int32_t)0x80000000; // 最大原始值
int32_t raw_first[10];              // 前10个原始值留档
uint32_t ads_test_count = 0;        // 当前已存样本数

void ads1292_parse_dma_buffer(void)
{
    uint8_t *buf = ads1292_dma_buf;

    /* 1. 帧校验 */
    if ((buf[0] & 0xF0) != 0xC0) {
        spi_error_count++;

        return;
    }

    /* 2. 解析通道 2 的 24 位有符号 ADC 原始值 */
    int32_t ch1_raw =
        ((int32_t)buf[6] << 16) |
        ((int32_t)buf[7] << 8)  |
        (int32_t)buf[8];

    /* 3. 24 位补码符号扩展 */
    if (ch1_raw & 0x800000) {
        ch1_raw |= (int32_t)0xFF000000;
    }

		
		
		
    /* 4. 将 ADC 原始值转换为电压，PGA 增益为 6 */
	float voltage =
			(float)ch1_raw * 2.42f / (6.0f * 8388607.0f);
		
		
		if (ch1_raw < raw_min) raw_min = ch1_raw;
if (ch1_raw > raw_max) raw_max = ch1_raw;

if (ads_test_count < 10) {
    raw_first[ads_test_count] = ch1_raw;
}

    /* 5. 保存到 RAM，最多保存 2500 个样本 */
    if (ads_test_count < TEST_SAMPLES) {
        ads_test_data[ads_test_count] = voltage;
        ads_test_count++;
    }
}


/* ================== 陷波器部分 ================== */
/* 采样率 500Hz, 中心频率 50Hz, Q值 30 */
static const float N50_B0 =  0.99029862f;
static const float N50_B1 = -1.60233682f;
static const float N50_B2 =  0.99029862f;
static const float N50_A1 = -1.60233682f;
static const float N50_A2 =  0.98059724f;

void Notch50_Reset(Notch50State *state)
{
    if (state != NULL) {
        state->z1 = 0.0f;
        state->z2 = 0.0f;
    }
}

float Notch50_Process(Notch50State *state, float input)
{
    float output;

    if (state == NULL) {
        return input;
    }

    // 核心 IIR 差分方程
    output = N50_B0 * input + state->z1;

    state->z1 = N50_B1 * input - N50_A1 * output + state->z2;
    state->z2 = N50_B2 * input - N50_A2 * output;

    return output;
}
/* ================================================ */

/* ================== 40Hz 低通滤波器 (Fs=500Hz) ================== */
// MATLAB: [b,a] = butter(2, 40/(500/2));
static const float LPF40_B0 = 0.20657208f;
static const float LPF40_B1 = 0.41314417f;
static const float LPF40_B2 = 0.20657208f;
static const float LPF40_A1 = -0.36952738f;
static const float LPF40_A2 = 0.19581571f;

void Lpf40_Reset(Lpf40State *state)
{
    if (state != NULL) {
        state->z1 = 0.0f;
        state->z2 = 0.0f;
    }
}

float Lpf40_Process(Lpf40State *state, float input)
{
    float output;

    if (state == NULL) {
        return input;
    }

    // 二阶 IIR 差分方程
    output = LPF40_B0 * input + state->z1;
    state->z1 = LPF40_B1 * input - LPF40_A1 * output + state->z2;
    state->z2 = LPF40_B2 * input - LPF40_A2 * output;

    return output;
}
/* ================================================================ */


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	    if (GPIO_Pin == GPIO_PIN_0) {					
					drdy_flag = 1; 
				
    }
}
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi1) {
			
        while (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY)) {}

        ads1292_cs(1);

        spi_rx_done = 1;
					
				ads_frame_count++; 
					
    }
}