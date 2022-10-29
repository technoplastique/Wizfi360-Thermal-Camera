#include "st7735.h"
#include "DEV_Config.h"
#include <stdlib.h>		
#include <stdio.h>
#include "ICM20948.h"
#include "arducampico.h"

uint8_t image[96*96*2];
IMU_EN_SENSOR_TYPE result;
IMU_ST_ANGLES_DATA Angles,  Gyro,  Acce, Magn;
struct arducam_config config;

void setup(){
	ST7735_Init();
	config.sccb = i2c0;
	config.sccb_mode = I2C_MODE_16_8;
	config.sensor_address = 0x24;
	config.pin_sioc = PIN_CAM_SIOC;
	config.pin_siod = PIN_CAM_SIOD;
	config.pin_resetb = PIN_CAM_RESETB;
	config.pin_xclk = PIN_CAM_XCLK;
	config.pin_vsync = PIN_CAM_VSYNC;
	config.pin_y2_pio_base = PIN_CAM_Y2_PIO_BASE;
	config.pio = pio0;
	config.pio_sm = 0;
	config.dma_channel = 0;
	arducam_init(&config);
	ST7735_FillScreen(ST7735_BLUE);
}

void loop(){
	
}
