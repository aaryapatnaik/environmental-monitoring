#include "bmp280.h"
#include <stdio.h>

static HAL_StatusTypeDef BMP280_ReadRegs(BMP280_HandleTypeDef *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(dev->hi2c, BMP280_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
}

static HAL_StatusTypeDef BMP280_WriteReg(BMP280_HandleTypeDef *dev, uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(dev->hi2c, BMP280_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100);
}

static HAL_StatusTypeDef BMP280_WriteRegVerified(BMP280_HandleTypeDef *dev, uint8_t reg, uint8_t value)
{
    for (int attempt = 0; attempt < 5; attempt++)
    {
        BMP280_WriteReg(dev, reg, value);
        HAL_Delay(2);
        uint8_t readback = 0xFF;
        BMP280_ReadRegs(dev, reg, &readback, 1);
        if (readback == value) return HAL_OK;
    }
    return HAL_ERROR;
}

HAL_StatusTypeDef BMP280_Init(BMP280_HandleTypeDef *dev, I2C_HandleTypeDef *hi2c)
{
    dev->hi2c = hi2c;

    uint8_t chip_id = 0;
    if (BMP280_ReadRegs(dev, BMP280_REG_CHIP_ID, &chip_id, 1) != HAL_OK) return HAL_ERROR;
    if (chip_id != BMP280_CHIP_ID) return HAL_ERROR;

    uint8_t calib[24];
    if (BMP280_ReadRegs(dev, BMP280_REG_CALIB_START, calib, 24) != HAL_OK) return HAL_ERROR;

    dev->calib.dig_T1 = (uint16_t)(calib[1]  << 8 | calib[0]);
    dev->calib.dig_T2 = (int16_t)(calib[3]   << 8 | calib[2]);
    dev->calib.dig_T3 = (int16_t)(calib[5]   << 8 | calib[4]);
    dev->calib.dig_P1 = (uint16_t)(calib[7]  << 8 | calib[6]);
    dev->calib.dig_P2 = (int16_t)(calib[9]   << 8 | calib[8]);
    dev->calib.dig_P3 = (int16_t)(calib[11]  << 8 | calib[10]);
    dev->calib.dig_P4 = (int16_t)(calib[13]  << 8 | calib[12]);
    dev->calib.dig_P5 = (int16_t)(calib[15]  << 8 | calib[14]);
    dev->calib.dig_P6 = (int16_t)(calib[17]  << 8 | calib[16]);
    dev->calib.dig_P7 = (int16_t)(calib[19]  << 8 | calib[18]);
    dev->calib.dig_P8 = (int16_t)(calib[21]  << 8 | calib[20]);
    dev->calib.dig_P9 = (int16_t)(calib[23]  << 8 | calib[22]);

    if (BMP280_WriteRegVerified(dev, BMP280_REG_CTRL_MEAS, 0x27) != HAL_OK) return HAL_ERROR;
    if (BMP280_WriteRegVerified(dev, BMP280_REG_CONFIG, 0x00) != HAL_OK) return HAL_ERROR;

    return HAL_OK;
}

static int32_t BMP280_CompensateTemp(BMP280_HandleTypeDef *dev, int32_t adc_T)
{
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)dev->calib.dig_T1 << 1))) * ((int32_t)dev->calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dev->calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)dev->calib.dig_T1))) >> 12) * ((int32_t)dev->calib.dig_T3)) >> 14;
    dev->t_fine = var1 + var2;
    return (dev->t_fine * 5 + 128) >> 8;   // 0.01 degC units
}

static uint32_t BMP280_CompensatePressure(BMP280_HandleTypeDef *dev, int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)dev->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dev->calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)dev->calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)dev->calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dev->calib.dig_P3) >> 8) + ((var1 * (int64_t)dev->calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dev->calib.dig_P1) >> 33;
    if (var1 == 0) return 0;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dev->calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dev->calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dev->calib.dig_P7) << 4);
    return (uint32_t)p;   // Q24.8 -> Pa = p / 256
}

HAL_StatusTypeDef BMP280_ReadData(BMP280_HandleTypeDef *dev, float *temperature_c, float *pressure_hpa)
{
    uint8_t raw[6];
    if (BMP280_ReadRegs(dev, BMP280_REG_PRESS_MSB, raw, 6) != HAL_OK) return HAL_ERROR;

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);

    int32_t  T = BMP280_CompensateTemp(dev, adc_T);
    uint32_t P = BMP280_CompensatePressure(dev, adc_P);

    *temperature_c = T / 100.0f;
    *pressure_hpa  = (P / 256.0f) / 100.0f;

    return HAL_OK;
}

void BMP280_DebugPrint(BMP280_HandleTypeDef *dev, UART_HandleTypeDef *huart)
{
    uint8_t raw[6];
    BMP280_ReadRegs(dev, BMP280_REG_PRESS_MSB, raw, 6);

    uint8_t ctrl_meas_readback = 0;
    uint8_t status = 0;
    BMP280_ReadRegs(dev, BMP280_REG_CTRL_MEAS, &ctrl_meas_readback, 1);
    BMP280_ReadRegs(dev, 0xF3, &status, 1);   // STATUS register

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);

    char buf[128];
    int len = snprintf(buf, sizeof(buf),
        "adc_T=%ld adc_P=%ld | ctrl_meas=0x%02X status=0x%02X\r\n",
        (long)adc_T, (long)adc_P, ctrl_meas_readback, status);
    HAL_UART_Transmit(huart, (uint8_t*)buf, len, 200);
}