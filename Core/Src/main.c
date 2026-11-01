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
        /* 陷波 + 20Hz 低通 */
        /* ========================================================= */

        float sum = 0.0f;
        for (uint32_t i = 0U; i < ads_test_count; i++) sum += ads_test_data[i];
        float mean = sum / (float)ads_test_count;

        /* 1. 50Hz 陷波器 */
        const float b0 =  0.9902986f;
        const float b1 = -1.6023368f;
        const float b2 =  0.9902986f;
        const float a1 = -1.6023368f;
        const float a2 =  0.9805972f;
        float nz1 = 0.0f, nz2 = 0.0f;

        /* 2. 10阶巴特沃斯低通 (截止频率 40Hz) */
        static const float lp_b[5][3] = {
            {0.00365247150f, 0.00730494300f, 0.00365247150f},
            {0.00365247150f, 0.00730494300f, 0.00365247150f},
            {0.00365247150f, 0.00730494300f, 0.00365247150f},
            {0.00365247150f, 0.00730494300f, 0.00365247150f},
            {0.00365247150f, 0.00730494300f, 0.00365247150f}
        };
        static const float lp_a1[5] = {-1.76565826028f, -1.78490427193f, -1.82269492520f, -1.87740488209f, -1.94607382805f};
        static const float lp_a2[5] = { 0.77969166059f,  0.79909063898f,  0.83718165126f,  0.89232644129f,  0.96154116603f};
        float lz1[5] = {0}, lz2[5] = {0};

        for (uint32_t i = 0U; i < ads_test_count; i++) {
            float x = ads_test_data[i] - mean; // 去直流
            
            /* 第一步：陷波 */
            float notch_out = b0 * x + nz1;
            nz1 = b1 * x - a1 * notch_out + nz2;
            nz2 = b2 * x - a2 * notch_out;

            /* 第二步：10 阶级联巴特沃斯低通 */
            float lp_out = notch_out;
            for (int j = 0; j < 5; j++) {
                float out_stage = lp_b[j][0] * lp_out + lz1[j];
                lz1[j] = lp_b[j][1] * lp_out - lp_a1[j] * out_stage + lz2[j];
                lz2[j] = lp_b[j][2] * lp_out - lp_a2[j] * out_stage;
                lp_out = out_stage;
            }

            ads_bp_notch[i] = lp_out; 
        }
				        /* 4. 打印数据（跳过前 250 个点避开初始震荡） */
        printf("--- Data Start ---\r\n");
        for (uint32_t i = 250U; i < ads_test_count; ++i) {
            printf("%.6f\r\n", ads_bp_notch[i]);
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
