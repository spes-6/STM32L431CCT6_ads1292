#ifndef __ADS1292_APP_H
#define __ADS1292_APP_H

#include "ADS1292.h"
#include "main.h"
#include <stdio.h>
#include "spi.h"

#include "usart.h"
#include "gpio.h"

#define TEST_SAMPLES 2500

#define ADS1292_BUF_SIZE 10

#define ADS1292_USE_RDATAC  1   // 1=连续模式，0=单次模式

typedef struct {
    uint32_t stat[ADS1292_BUF_SIZE];
    float ch1_data[ADS1292_BUF_SIZE];
} ads1292_data_str;


extern ads1292_data_str ads1292_data;
extern uint16_t ads1291_count;
extern uint8_t sam_sta;


void ads1292_parse_dma_buffer(void);

/* 陷波器 */
typedef struct {
    float z1;
    float z2;
} Notch50State;

extern float ads_test_data[TEST_SAMPLES];
extern float ads_bp_notch[TEST_SAMPLES];
void Notch50_Reset(Notch50State *state);
float Notch50_Process(Notch50State *state, float input);
// 40hz低通：
typedef struct {
    float z1;
    float z2;
} Lpf40State;

void Lpf40_Reset(Lpf40State *state);
float Lpf40_Process(Lpf40State *state, float input);

#endif