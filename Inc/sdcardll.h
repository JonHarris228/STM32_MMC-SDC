#ifndef __SDCARDLL_H__
#define __SDCARDLL_H__

#include "stm32f1xx_ll_spi.h"

#define SDCARD_SPI_PORT      SPI1
#define SDCARD_SS_Pin        0b10000
#define SDCARD_SS_GPIO_Port  GPIOA

#define SDCARD_MAX_LOOP		 256

#define DATA_TOKEN_CMD9  0xFE
#define DATA_TOKEN_CMD17 0xFE
#define DATA_TOKEN_CMD18 0xFE
#define DATA_TOKEN_CMD24 0xFE
#define DATA_TOKEN_CMD25 0xFC

/*
R1: 0abcdefg
     ||||||`- 1th bit (g): card is in idle state
     |||||`-- 2th bit (f): erase sequence cleared
     ||||`--- 3th bit (e): illigal command detected
     |||`---- 4th bit (d): crc check error
     ||`----- 5th bit (c): error in the sequence of erase commands
     |`------ 6th bit (b): misaligned addres used in command
     `------- 7th bit (a): command argument outside allowed range
             (8th bit is always zero)
*/

int SDCard_Init();
int SDCard_BlocksNum(uint32_t *num);
int SDCard_ReadSingle(uint32_t block_n, uint8_t data[512]);
int SDCard_WriteSingle(uint32_t block_n, uint8_t data[512]);

static uint8_t SDCard_SendComand(uint8_t comand, uint32_t arg, uint8_t crc);
static uint8_t SDCard_WaitToken(uint8_t token);
static void SDCard_BusyWait();

#endif
