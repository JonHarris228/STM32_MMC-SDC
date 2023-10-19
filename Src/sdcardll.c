#include "sdcardll.h"

int SDCard_Init(){
	// Variables declaration
	uint32_t r7;
	uint8_t r1;
	// Enable SPI (SPE in SPI -> CR1 register)
	if((SDCARD_SPI_PORT -> CR1 & 1 << 6) != 1 << 6){
		SDCARD_SPI_PORT -> CR1 |= 1 << 6;
	}

	// Unselect SS pin
	SDCARD_SS_GPIO_Port -> ODR |= SDCARD_SS_Pin;

	// 74+ pulse to SCLK
	for (int i=0; i<10; i++){
		LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
	}

	// Select SS pin
	SDCARD_SS_GPIO_Port -> ODR &= ~SDCARD_SS_Pin;

	// Busy check
	SDCard_BusyWait();

	// Send CMD0
	if (SDCard_SendComand(0, 0x00000000, 0x4A) != 1) return -1;

	// Send CMD8
	if (SDCard_SendComand(8, 0x000001AA, 0x43) != 1) return -2;
	else {
		for (int i = 3; i>=0; i--){
			LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
			r7 |= LL_SPI_ReceiveData8(SDCARD_SPI_PORT) << i*8;
		}
		if (!((r7 & 0x0000FF00) >> 8 == 0x01 && (r7 & 0x000000FF) >> 0 == 0xAA)) return -3;
	}

	// Send ACMD41 (Send CMD55 before!!! Read doc)
	if (SDCard_SendComand(55, 0x00000000, 0) != 1) return -4;
	r1 = SDCard_SendComand(41, 0x40000000, 0);
	if (r1 == 0x00){}
	else if (r1 == 0x01){
		for (int i = 0; i < SDCARD_MAX_LOOP; i++){
			if (SDCard_SendComand(55, 0x00000000, 0) != 1) return -5;
			if (i == SDCARD_MAX_LOOP - 1) return -6;
			if (SDCard_SendComand(41, 0x40000000, 0) == 0x00) break;
		}
	}

	// Send CMD58
	r7 = 0;
	if (SDCard_SendComand(58, 0x00000000, 0) != 0) return -7;
		else {
			// Read r7
			for (int i = 3; i>=0; i--){
				LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
				r7 |= LL_SPI_ReceiveData8(SDCARD_SPI_PORT) << i*8;
			}
			if ((r7 & 0xC0000000) >> 24 == 0xC0) {}
			else if((r7 & 0x80000000) >> 24 == 0x80){
				// Send CMD16
				if (SDCard_SendComand(16, 0x00000200, 0) != 0) return -8;
			}
			else return -9;
	}
	SDCARD_SS_GPIO_Port -> ODR |= SDCARD_SS_Pin;
	return 1;
}


int SDCard_BlocksNum(uint32_t *num){
	// Variables declaration
	uint8_t csd[16], crc[2];
	uint32_t buff;
	// Select SS pin
	SDCARD_SS_GPIO_Port -> ODR &= ~SDCARD_SS_Pin;
	// Busy check
	SDCard_BusyWait();
	// Send CMD9
	if (SDCard_SendComand(9, 0x00000000, 0) != 0) return -1;
	// Read Token
	if (SDCard_WaitToken(DATA_TOKEN_CMD9) != 1) return -2;
	// Read CSD
	for(int i =0; i<16; i++){
		LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
		csd[i] = LL_SPI_ReceiveData8(SDCARD_SPI_PORT);
	}
	// Read CRC
	for(int i =0; i<2; i++){
		LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
		crc[i] = LL_SPI_ReceiveData8(SDCARD_SPI_PORT);
	}
	// Formula for block numbers
	buff = csd[7] & 0x3F;
	buff = (buff << 8) | csd[8];
	buff = (buff << 8) | csd[9];
	buff = (buff + 1) << 10;
	*num = buff;

	SDCARD_SS_GPIO_Port -> ODR |= SDCARD_SS_Pin;
	return 1;
}


int SDCard_ReadSingle(uint32_t block_n, uint8_t data[512]){
	// Variables declaration
	uint8_t crc[2];
	// Select SS
	SDCARD_SS_GPIO_Port -> ODR &= ~SDCARD_SS_Pin;
	// Busy check
	SDCard_BusyWait();
	// Send CMD17 with address in argument(must be 0 or multiple of 512)
	if (SDCard_SendComand(17, block_n, 0) != 0) return -1;
	// Read Token
	if (SDCard_WaitToken(DATA_TOKEN_CMD17) != 1) return -2;
	// Read data
	for (int i = 0; i <512; i++){
		LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
		data[i] = LL_SPI_ReceiveData8(SDCARD_SPI_PORT);
	}
	// Read CRC
	for (int i =0; i<2; i++){
		LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
		crc[i] = LL_SPI_ReceiveData8(SDCARD_SPI_PORT);
	}
	return 1;
}


int SDCard_WriteSingle(uint32_t block_n, uint8_t data[512]){
	// Variables declaration
	uint8_t resp;
	// Select SS
	SDCARD_SS_GPIO_Port -> ODR &= ~SDCARD_SS_Pin;
	// Busy check
	SDCard_BusyWait();
	// Send CMD24 with address in argument(must be 0 or multiple of 512)
	if (SDCard_SendComand(24, block_n, 0) != 0) return -1;
	// Send Data Packet
	LL_SPI_TransmitData8(SDCARD_SPI_PORT, DATA_TOKEN_CMD24);
	for (int i = 0; i <512; i++){
		LL_SPI_TransmitData8(SDCARD_SPI_PORT, data[i]);
	}
	LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
	LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
	// Get Data Response
	for(int i = 0; i < SDCARD_MAX_LOOP; i++){
		LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
		resp = LL_SPI_ReceiveData8(SDCARD_SPI_PORT);
		if((resp & 0b00000101) == 0b101) {
			return 1;
		}
	}
	return -2;
}



static uint8_t SDCard_SendComand(uint8_t comand, uint32_t arg, uint8_t crc){
	uint8_t r1;
	LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0x40 | comand);
	LL_SPI_TransmitData8(SDCARD_SPI_PORT, (arg & 0xFF000000) >> 24);
	LL_SPI_TransmitData8(SDCARD_SPI_PORT, (arg & 0x00FF0000) >> 16);
	LL_SPI_TransmitData8(SDCARD_SPI_PORT, (arg & 0x0000FF00) >> 8);
	LL_SPI_TransmitData8(SDCARD_SPI_PORT, (arg & 0x000000FF) >> 0);
	LL_SPI_TransmitData8(SDCARD_SPI_PORT, (crc << 1) | 1);
	for(int i = 0; i < SDCARD_MAX_LOOP; i++){
		LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
		r1 = LL_SPI_ReceiveData8(SDCARD_SPI_PORT);
		if(r1 != 0xFF) {
			return r1;
		}
	}
	return 0xFF;
}

static uint8_t SDCard_WaitToken(uint8_t token){
	uint8_t r1;
	for(int i = 0; i < SDCARD_MAX_LOOP; i++){
		LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
		r1 = LL_SPI_ReceiveData8(SDCARD_SPI_PORT);
		if(r1 == token) return 1;
		else if (r1 != 0xFF) return -1;
	}
	return -2;
}

static void SDCard_BusyWait(){
	uint8_t data;
	for(;;){
		LL_SPI_TransmitData8(SDCARD_SPI_PORT, 0xFF);
		data = LL_SPI_ReceiveData8(SDCARD_SPI_PORT);
		if (data == 0xFF) break;
	}
}
