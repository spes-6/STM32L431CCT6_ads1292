#include "ADS1292.h"


 uint8_t dummy[9] = {0};
 uint8_t ads1292_dma_buf[9];


void ads1292_cs(uint8_t x)
{
	x?HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET)
	:HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
}


void ads1292_WREG(uint8_t addr,uint8_t cnt,uint8_t *Data)
{
	ads1292_cs(0);
	uint8_t txData[2];
	txData[0]=0x40+addr;
	txData[1]=cnt-1;
	HAL_SPI_Transmit(&hspi1,txData,2,100);
	HAL_Delay(1);
	HAL_SPI_Transmit(&hspi1,Data,cnt,100);
	ads1292_cs(1);
	
}

void ads1292_RREG(uint8_t addr,uint8_t cnt,uint8_t *Data)
{
	ads1292_cs(0);
	
	uint8_t txData[3];
	txData[0] = 0x11;                    // SDATAC
	txData[1]=0x20+addr;
	txData[2]=cnt-1;
	uint8_t dummy[10]={0};
	
	HAL_SPI_TransmitReceive(&hspi1,txData,dummy,3,100);
	HAL_Delay(1);
	

	HAL_SPI_TransmitReceive(&hspi1,dummy,Data,cnt,100);

	ads1292_cs(1);
}
//void ads1292_SendSDATAC(void)
//{
//    uint8_t cmd = 0x11;

//    ads1292_cs(0);
//    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
//    ads1292_cs(1);

//    HAL_Delay(1);
//}


void ads1292_cmd(uint8_t cmd)
{

	HAL_SPI_Transmit(&hspi1,&cmd,1,100);

}

void ads1292_Init(void)
{

    HAL_Delay(50);
    
    // 2. START 拉低
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(10);
    
    // 3. 复位
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
	 HAL_Delay(10);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
		HAL_Delay(10);

    
    
    // 4. 发 SDATAC
    ads1292_cs(0);
    ads1292_cmd(0x11);
    ads1292_cs(1);
    HAL_Delay(1);
    
    // 5. 配置所有寄存器（从 0x01 到 0x0B，共 11 个）
		uint8_t regData[10];
		regData[0] = 0x02U;   // 0x01 CONFIG1  采样率 500SPS
		regData[1] = 0x20U;    // 0x02 CONFIG2 方波内部测试信号
		regData[2] = 0x10U;   // 0x03 LOFF导联脱落检测控制寄存器（默认）
		regData[3] = 0x81U;     // 0x04 CH1SET
		regData[4] = 0x00U;   // 0x05 CH2SET
		regData[5] = 0x00;   // 0x06 RLD_SENS
		regData[6] = 0x00;   // 0x07 LOFF_SENS
		regData[7] = 0x40U;   // 0x08 LOFF_STAT（配置 CLK_DIV=1，晶振是 2.048M）
		regData[8] = 0x02;   // 0x09 RESP1
		regData[9] = 0x07;   // 0x0A RESP2

		ads1292_WREG(0x01, 10, regData);  // 从 0x01 写 10 个，覆盖 0x01~0x0A
		uint8_t gpio_cfg = 0x0C;
		ads1292_WREG(0x0B, 1, &gpio_cfg);
    
		HAL_Delay(150);   // 等待内部参考稳定


    
    // 6. START 拉高
		ads1292_cs(0);
    ads1292_cmd(0x08);   // START 命令
		 ads1292_cs(1);
    HAL_Delay(10);

    // 7. 发 RDATAC
    ads1292_cs(0);
    ads1292_cmd(0x10);
    ads1292_cs(1);
    HAL_Delay(1);
    
    printf("RDATAC sent\r\n");
}


void ads1292_RDATA(uint8_t *Data,uint8_t len)
{
	ads1292_cs(0);
	ads1292_cmd(0x12);
	uint8_t dummy[9] = {0};
//	HAL_Delay(1); 
	HAL_SPI_TransmitReceive(&hspi1, dummy, Data, len, 100);
	ads1292_cs(1);
}

/* ===== 启动一次 DMA 读取 ===== */
void ads1291_start_read(void)
{
    ads1292_cs(0);
    HAL_SPI_TransmitReceive_DMA(&hspi1, dummy,
                                ads1292_dma_buf, 9);
}