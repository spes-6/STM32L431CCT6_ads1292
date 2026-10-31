#include "ADS1292_App.h"

volatile uint8_t drdy_flag = 0;


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	    if (GPIO_Pin == GPIO_PIN_0) {					
					drdy_flag = 1; 
    }
}