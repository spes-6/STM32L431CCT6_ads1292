/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ADS1292.h"
#include <stdio.h>
#include "ADS1292_App.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern volatile uint8_t drdy_flag;
extern volatile uint8_t spi_rx_done ;

extern int32_t raw_min;       // 最小原始值
extern int32_t raw_max ; // 最大原始值
extern int32_t raw_first[10];              // 前10个原始值留档
extern uint32_t ads_test_count;        // 当前已存样本数

uint8_t sam_sta=0;
extern  float ads_test_data[TEST_SAMPLES];
uint8_t output_done = 0;
static float ads_test_filtered[2500];
static float ads_bp_notch[2500];


typedef struct
{
    float z1;
    float z2;
} BiquadState;

float Biquad_Process(
    float x,
    float b0, float b1, float b2,
    float a1, float a2,
    BiquadState *s)
{
    float y = b0 * x + s->z1;

    s->z1 = b1 * x - a1 * y + s->z2;
    s->z2 = b2 * x - a2 * y;

    return y;
}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//* printf �ض��� RS485��dr485: ��=���ͣ���=���գ� */
int fputc(int ch, FILE *f)
{
    
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
    //HAL_Delay(1);   

   
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);

   
    while (!(USART1->ISR & USART_ISR_TC));

    
//    HAL_Delay(1);   
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);

    return ch;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	
 ads1292_Init();



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (drdy_flag) {
        drdy_flag = 0;
        ads1291_start_read();   
    }	
    if (spi_rx_done) {
    spi_rx_done = 0;
    ads1292_parse_dma_buffer();
        if (ads_test_count >= TEST_SAMPLES) {
            sam_sta = 1;
        }
			}

			/* 3. 采满后只打印一次 */
    if (sam_sta) {
        sam_sta = 0;

        /* 关 DRDY 中断 */
        HAL_NVIC_DisableIRQ(EXTI0_IRQn);

        /* 停止 ADS1292 连续读取 */
        ads1292_cs(1);   // 没有这个函数就拉高 CS
//--------------------------------------------------------------------------
//       /* 2. 初始化滤波器 */
//        Notch50State notch50_bp;
//        Lpf40State   lpf40_bp;
//        Notch50_Reset(&notch50_bp);
//        Lpf40_Reset(&lpf40_bp);
//        
//        /* 3. 执行级联滤波：先过 50Hz 陷波，再过 40Hz 低通 */
//        float temp_val;
//        for (uint32_t i = 0U; i < ads_test_count; i++) {
//            // 第一步：打掉 50Hz 及其带内干扰
//            temp_val = Notch50_Process(&notch50_bp, ads_test_data[i]);
//            // 第二步：切掉 40Hz 以上的所有高频谐波(100Hz/150Hz等)和毛刺
//            ads_bp_notch[i] = Lpf40_Process(&lpf40_bp, temp_val);
//        }

        /* ========================================================= */
 
        /* ========================================================= */
        
BiquadState hp_state = {0};
BiquadState notch_state = {0};
BiquadState lp_state = {0};
BiquadState notch100_state = {0};
for (uint32_t i = 0; i < ads_test_count; i++)
{
    float x = ads_test_data[i];

// 1. 5Hz 高通
float hp_out = Biquad_Process(
    x,
    0.95654323f,
   -1.91308645f,
    0.95654323f,
    -1.91119707f,
    0.91497583f,
    &hp_state
);

// 2. 49.967Hz 陷波，Q=10
float notch50_out = Biquad_Process(
    hp_out,
    0.97146564f,
   -1.57233788f,
    0.97146564f,
   -1.57233788f,
    0.94293128f,
    &notch_state
);

// 3. 100Hz 陷波
float notch100_out = Biquad_Process(
    notch50_out,
    0.98439639f,
   -0.60839043f,
    0.98439639f,
   -0.60839043f,
    0.96879278f,
    &notch100_state
);

// 4. 20Hz 低通
float lp_out = Biquad_Process(
    notch100_out,
    0.01335920f,
    0.02671840f,
    0.01335920f,
   -1.64745998f,
    0.70089678f,
    &lp_state
);

ads_bp_notch[i] = lp_out;
}
				        /* 4. 打印数据（跳过前 250 个点避开初始震荡） */
        printf("--- Data Start ---\r\n");
        for (uint32_t i = 250U; i < ads_test_count; ++i) {
            printf("%.9e\r\n", ads_bp_notch[i]);
//					printf("%.6f\r\n", ads_test_data[i]);
        }
        printf("--- Data End ---\r\n");

        /* 停在这里，不再采集 */
        while (1) {
            HAL_Delay(1000);
        

				}}
	
}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
