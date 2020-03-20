#include <stm32f4xx.h>
#include "cmsis_os.h"
#define N 14

uint8_t DUMMY_BYTE = 0x00;
uint8_t RDSR_ADDR = 0x05; 		// Read-Status-Register
uint8_t BYTE_PROGRAM = 0x02;
uint8_t READ = 0x03;
uint8_t WREN = 0x06; // WRITE_ENABLE
uint8_t WRSR = 0x01; // WRITE_STATUS_REGISTER
uint8_t SECTOR_ERASE = 0x20;

static uint8_t status;
static uint8_t old_loaded_data;
static uint8_t loaded_data;

/* Memory activation (using CS) */
void h_drv_SPI_CS_Enable (void)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_7);
}


/* Memory deactivation (using CS) */
void h_drv_SPI_CS_Disable (void)
{
	GPIO_SetBits(GPIOD, GPIO_Pin_7);
}

/* Reading/writing one byte using SPI */
uint8_t h_drv_SPI_Write_Byte (uint8_t Data)
{
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET){}
  SPI_I2S_SendData(SPI1, Data);
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET){}
  return SPI_I2S_ReceiveData(SPI1);
}


/* Reading status register */
uint8_t h_drv_Read_Status_Register (void)
{
	uint8_t Status = 0;
	
	h_drv_SPI_CS_Enable();
	h_drv_SPI_Write_Byte(RDSR_ADDR);
	Status = h_drv_SPI_Write_Byte(DUMMY_BYTE);
	h_drv_SPI_CS_Disable();
	return Status;
}

/* Writing one byte with Byte-Program */
void h_drv_Byte_Program_Write_Byte (uint32_t ADD, uint8_t Data)
{
	h_drv_SPI_CS_Enable();
	h_drv_SPI_Write_Byte(WREN);
	h_drv_SPI_CS_Disable();
	
	h_drv_SPI_CS_Enable();
	h_drv_SPI_Write_Byte(BYTE_PROGRAM);
	
	uint8_t* currAdd;
	currAdd = &ADD;
	currAdd++;
	h_drv_SPI_Write_Byte(*currAdd); // First byte of address [A23-A16]
	currAdd++;
	h_drv_SPI_Write_Byte(*currAdd); // Second byte of address [A15-A8]
	currAdd++;
	h_drv_SPI_Write_Byte(*currAdd); // Third byte of address [A7-A0]
	
	h_drv_SPI_Write_Byte(Data);
	h_drv_SPI_CS_Disable();
}


/* Reading one byte */
uint8_t h_drv_Read_Byte (uint32_t* ADD)
{
	uint8_t data;
	
	h_drv_SPI_CS_Enable();
	h_drv_SPI_Write_Byte(READ);
	
	uint8_t* currAdd;
	currAdd = &ADD;
	currAdd++;
	h_drv_SPI_Write_Byte(*currAdd); // First byte of address [A23-A16]
	currAdd++;
	h_drv_SPI_Write_Byte(*currAdd); // Second byte of address [A15-A8]
	currAdd++;
	h_drv_SPI_Write_Byte(*currAdd); // Third byte of address [A7-A0]
	
	data = h_drv_SPI_Write_Byte(DUMMY_BYTE);
	h_drv_SPI_CS_Disable();
	
	return data;
}


/* Allow data writing */
void h_drv_Allow_Data_Writing ()
{
	h_drv_SPI_CS_Enable();
	h_drv_SPI_Write_Byte(WREN);
	h_drv_SPI_CS_Disable();
	
	h_drv_SPI_CS_Enable();
	h_drv_SPI_Write_Byte(WRSR);
	h_drv_SPI_Write_Byte(0x00);
	h_drv_SPI_CS_Disable();
}


/* 4-KByte Sector-Erase */
void h_drv_Sector_Erase (uint32_t* ADD)
{
	h_drv_SPI_CS_Enable();
	h_drv_SPI_Write_Byte(WREN);
	h_drv_SPI_CS_Disable();
	
	h_drv_SPI_CS_Enable();
	h_drv_SPI_Write_Byte(SECTOR_ERASE);
	
	uint8_t* currAdd;
	currAdd = &ADD;
	currAdd++;
	h_drv_SPI_Write_Byte(*currAdd); // First byte of address [A23-A16]
	currAdd++;
	h_drv_SPI_Write_Byte(*currAdd); // Second byte of address [A15-A8]
	currAdd++;
	h_drv_SPI_Write_Byte(*currAdd); // Third byte of address [A7-A0]
	
	h_drv_SPI_CS_Disable();
}


void next_Add(uint32_t* ADD){
	*ADD += 8;
}


/* Write array of bytes */
void h_drv_Write_Array (uint32_t* ADD, uint8_t* Data)
{
	for(int i = 0; i < N; i++){
		h_drv_Byte_Program_Write_Byte(*ADD, Data[i]);
		next_Add(ADD);
	}
}

void Init(){
	GPIO_InitTypeDef 	GPIO_Init_LED;
	SPI_InitTypeDef		SPI_Init_user;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
	GPIO_Init_LED.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init_LED.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init_LED.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init_LED.GPIO_OType = GPIO_OType_PP;
	GPIO_Init_LED.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_Init_LED);
	
	GPIO_Init_LED.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5;
	GPIO_Init_LED.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init_LED.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init_LED.GPIO_OType = GPIO_OType_PP;
	GPIO_Init_LED.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_Init_LED);
	
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);
	
	GPIO_Init_LED.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init_LED.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init_LED.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init_LED.GPIO_OType = GPIO_OType_PP;
	GPIO_Init_LED.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_Init_LED);
	
	h_drv_SPI_CS_Disable();
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	SPI_Init_user.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_Init_user.SPI_Mode = SPI_Mode_Master;
	SPI_Init_user.SPI_DataSize = SPI_DataSize_8b;
	SPI_Init_user.SPI_CPOL = SPI_CPOL_High;
	SPI_Init_user.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_Init_user.SPI_NSS = SPI_NSS_Soft;
	SPI_Init_user.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
	SPI_Init_user.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_Init_user.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_Init_user);
	SPI_Cmd(SPI1, ENABLE);
}


int main (void){	
	Init();
	
	// Allow data writing to all sectors of memory
	h_drv_Allow_Data_Writing();
	status = h_drv_Read_Status_Register();
	
	// Erasing sector before writing
	h_drv_Sector_Erase(0x000000);
	
	// Writing data
	uint8_t data[N] = {0x53, 0x5E, 0x5C, 0x5D, 0x14, 0x5C, 0x58, 
										 0x56, 0x4E, 0x14, 0x5D, 0x4E, 0x61, 0x5D};
	
 	h_drv_Write_Array(0x000000, data);
}

