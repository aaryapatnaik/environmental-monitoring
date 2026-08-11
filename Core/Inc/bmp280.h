#ifndef BMP280_H
#define BMP280_H
 
#include "stm32f4xx_hal.h"
#include <stdint.h>
 
#define BMP280_I2C_ADDR         (0x76 << 1)   // try (0x77 << 1) if this doesn't ACK
 
#define BMP280_REG_CHIP_ID      0xD0
#define BMP280_REG_CTRL_MEAS    0xF4
#define BMP280_REG_CONFIG       0xF5
#define BMP280_REG_STATUS       0xF3
#define BMP280_REG_PRESS_MSB    0xF7
#define BMP280_REG_CALIB_START  0x88
 
#define BMP280_CHIP_ID          0x58
 
// ctrl_meas: normal mode, temp oversampling x1, pressure oversampling x1
#define BMP280_CTRL_MEAS_NORMAL_MODE  0x27
// config: no IIR filter, shortest standby time between samples
#define BMP280_CONFIG_NO_FILTER       0x00
 
// per-chip values read once at init, hardcoding them would break on a different sensor
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_CalibData;
 
typedef struct {
    I2C_HandleTypeDef *hi2c;
    BMP280_CalibData calib;
    int32_t t_fine; // running temp value the pressure compensation needs too
} BMP280_HandleTypeDef;
 
// checks chip id first so a wiring mistake fails loudly instead of returning garbage data
HAL_StatusTypeDef BMP280_Init(BMP280_HandleTypeDef *dev, I2C_HandleTypeDef *hi2c);
// reads and compensates one temp/pressure sample
HAL_StatusTypeDef BMP280_ReadData(BMP280_HandleTypeDef *dev, float *temperature_c, float *pressure_hpa);
// dumps raw adc values and a couple status registers over uart for debugging
void BMP280_DebugPrint(BMP280_HandleTypeDef *dev, UART_HandleTypeDef *huart);
 
#endif