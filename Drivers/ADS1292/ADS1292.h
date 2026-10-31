#ifndef ADS1292_H
#define ADS1292_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include "spi.h"

///* Command constants used by the original ads1291.c. */
//#define WAKEUP  0x02U
//#define STANDBY 0x04U
//#define RESET   0x06U
//#define START   0x08U
//#define STOP    0x0AU
//#define RDATAC  0x10U
//#define SDATAC  0x11U
//#define RDATA   0x12U

//extern volatile uint8_t dma_done;
//extern volatile uint8_t ads1291_dma_buf[6];
//extern volatile uint8_t spi_busy;

void ads1292_cs(uint8_t x);
//void ads1291_WREG(uint8_t addr, uint8_t cnt, uint8_t *Data);
void ads1292_RREG(uint8_t addr, uint8_t cnt, uint8_t *Data);
void ads1292_cmd(uint8_t cmd);
void ads1292_Init(void);
//void ads1291_Reset(void);
void ads1292_RDATA(uint8_t *Data, uint8_t len);
//void ads1292_RDATAC(void);
//void ads1292_start_continuous(void);
//void ads1292_dump_regs(void);
//void ads1292_SendSDATAC(void);
void ads1291_start_read(void);

#ifdef __cplusplus
}
#endif
#endif /* ADS1291_H */

