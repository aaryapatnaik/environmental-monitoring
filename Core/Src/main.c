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
#include <stdio.h>
#include "stm32f4xx_hal_adc.h"
#include <string.h>
#include "timer.h"
#include "bmp280.h"
#include "lcd.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
    SYS_OK = 0,
    SYS_ALERT,
    SYS_SENSOR_ERROR
} SystemState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TEMP_THRESHOLD_MIN_C   15.0f
#define TEMP_THRESHOLD_MAX_C   35.0f
#define TEMP_THRESHOLD_HYST_C  1.0f
#define ADC_MAX_VALUE          4095.0f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
uint8_t rx_byte;
char cmd_buf[64];
uint8_t cmd_pos = 0;
uint8_t cmd_ready = 0;

BMP280_HandleTypeDef bmp280;
static SystemState_t current_state = SYS_OK;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */
static void MX_I2C1_Init(void);
void Sensors_Sample_And_Report(void);
static float Threshold_FromADC(uint32_t adc_val);
static SystemState_t FSM_NextState(SystemState_t state, uint8_t bmp_ok, float temp_c, float threshold_c);
static const char* FSM_StateName(SystemState_t state);
static const char* FSM_StateNameShort(SystemState_t state);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_USART2_UART_Init();
  MX_ADC1_Init();

  /* USER CODE BEGIN 2 */
  MX_I2C1_Init();
  LCD_Init();
  

  uint8_t bmp_ok = 0;
  for (int i = 0; i < 5 && !bmp_ok; i++) {
      HAL_Delay(50);
      if (BMP280_Init(&bmp280, &hi2c1) == HAL_OK) bmp_ok = 1;
  }
  if (bmp_ok) {
      HAL_UART_Transmit(&huart2, (uint8_t*)"BMP280 init OK\r\n", 16, 100);
  }
  else {
      HAL_UART_Transmit(&huart2, (uint8_t*)"BMP280 init FAILED\r\n", 20, 100);
  }
  Sample_Timer_Init();
  // HAL_UART_Transmit(&huart2, (uint8_t*)"Timer init called\r\n", 19, 100);

  uint8_t startup[] = "UART ready\r\n> ";
  HAL_UART_Transmit(&huart2, startup, sizeof(startup)-1, 100);
  HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

  HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    if (cmd_ready) {
        cmd_ready = 0;

        if (strcmp(cmd_buf, "HI") == 0)
            HAL_UART_Transmit(&huart2, (uint8_t*)"HELLO\r\n> ", 9, 100);
        else if (strcmp(cmd_buf, "LED ON") == 0)
        {
            HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
            HAL_UART_Transmit(&huart2, (uint8_t*)"LED ON\r\n> ", 10, 100);
        }
        else if (strcmp(cmd_buf, "LED OFF") == 0)
        {
            HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
            HAL_UART_Transmit(&huart2, (uint8_t*)"LED OFF\r\n> ", 11, 100);
        }
        else if (strcmp(cmd_buf, "READ_ADC") == 0)   // <-- add this block
        {
          HAL_ADC_Start(&hadc1);
          HAL_ADC_PollForConversion(&hadc1, 100);
          uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
          HAL_ADC_Stop(&hadc1);

          char adc_str[32];
          int len = snprintf(adc_str, sizeof(adc_str), "ADC: %lu\r\n> ", adc_val);
          HAL_UART_Transmit(&huart2, (uint8_t*)adc_str, len, 100);
        }
        else if (strcmp(cmd_buf, "READ_BMP") == 0)
        {
          // float temp_c, press_hpa;
          // if (BMP280_ReadData(&bmp280, &temp_c, &press_hpa) == HAL_OK)
          // {
          //   char bmp_str[64];
          //   int len = snprintf(bmp_str, sizeof(bmp_str), "Temp: %.2f C, Pressure: %.2f hPa\r\n> ", temp_c, press_hpa);
          //   HAL_UART_Transmit(&huart2, (uint8_t*)bmp_str, len, 100);
          // }
          // else
          //   HAL_UART_Transmit(&huart2, (uint8_t*)"BMP280 read error\r\n> ", 21, 100);
          Sensors_Sample_And_Report();
        }
        else if (strcmp(cmd_buf, "DEBUG_BMP") == 0)
        {
          BMP280_DebugPrint(&bmp280, &huart2);
          HAL_UART_Transmit(&huart2, (uint8_t*)"> ", 2, 100);
        }
        else
            HAL_UART_Transmit(&huart2, (uint8_t*)"Unknown command\r\n> ", 18, 100);

        cmd_pos = 0;
        memset(cmd_buf, 0, sizeof(cmd_buf));
    }
    if (sample_ready) {
      sample_ready = 0;
      Sensors_Sample_And_Report();
    }
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void) // i2c1 init
{
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;   // PB8 = SCL, PB9 = SDA
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

static void MX_ADC1_Init(void)
{
    // Enable clocks for ADC1 and GPIOA
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE(); // already enabled, but harmless

    // Configure PA0 as analog input
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure ADC1
    hadc1.Instance = ADC1;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    // Configure channel 0
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (rx_byte == '\r' || rx_byte == '\n')
        {
            cmd_buf[cmd_pos] = '\0';
            cmd_ready = 1;
        }
        else if (cmd_pos < 63)
        {
            cmd_buf[cmd_pos++] = rx_byte;
        }

        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}

// void Sensors_Sample_And_Report(void)
// {
//     float temp_c, press_hpa;
//     uint32_t adc_val = 0;

//     HAL_ADC_Start(&hadc1);
//     HAL_ADC_PollForConversion(&hadc1, 100);
//     adc_val = HAL_ADC_GetValue(&hadc1);
//     HAL_ADC_Stop(&hadc1);

//     if (BMP280_ReadData(&bmp280, &temp_c, &press_hpa) == HAL_OK)
//     {
//         char out_str[96];
//         int len = snprintf(out_str, sizeof(out_str),
//             "Temp: %.2f C, Pressure: %.2f hPa, ADC: %lu\r\n> ",
//             temp_c, press_hpa, (unsigned long)adc_val);
//         HAL_UART_Transmit(&huart2, (uint8_t*)out_str, len, 100);

//         // printing to lcd
//         char lcd_line1[17], lcd_line2[17];
//         snprintf(lcd_line1, sizeof(lcd_line1), "Temp: %.1f C", temp_c);
//         snprintf(lcd_line2, sizeof(lcd_line2), "Press: %.0f hPa", press_hpa);
//         lcd_print_line(0, lcd_line1);
//         lcd_print_line(1, lcd_line2);
//     }
//     else
//     {
//         HAL_UART_Transmit(&huart2, (uint8_t*)"BMP280 read error\r\n> ", 21, 100);
//     }
// }
// Maps a 0-4095 ADC reading linearly onto the [TEMP_THRESHOLD_MIN_C, TEMP_THRESHOLD_MAX_C] range.
static float Threshold_FromADC(uint32_t adc_val)
{
    float frac = (float)adc_val / ADC_MAX_VALUE;
    return TEMP_THRESHOLD_MIN_C + frac * (TEMP_THRESHOLD_MAX_C - TEMP_THRESHOLD_MIN_C);
}

// Pure transition function: given the current state and this sample's inputs, returns the next state.
// Hysteresis on the ALERT->OK edge prevents chatter when temp_c sits right at threshold_c.
static SystemState_t FSM_NextState(SystemState_t state, uint8_t bmp_ok, float temp_c, float threshold_c)
{
    if (!bmp_ok)
    {
        return SYS_SENSOR_ERROR;
    }

    switch (state)
    {
        case SYS_OK:
            return (temp_c >= threshold_c) ? SYS_ALERT : SYS_OK;

        case SYS_ALERT:
            return (temp_c < threshold_c - TEMP_THRESHOLD_HYST_C) ? SYS_OK : SYS_ALERT;

        case SYS_SENSOR_ERROR:
        default:
            // BMP recovered this sample; re-evaluate fresh against the threshold.
            return (temp_c >= threshold_c) ? SYS_ALERT : SYS_OK;
    }
}

static const char* FSM_StateName(SystemState_t state)
{
    switch (state)
    {
        case SYS_OK:            return "OK";
        case SYS_ALERT:         return "ALERT";
        case SYS_SENSOR_ERROR:  return "SENSOR_ERROR";
        default:                return "UNKNOWN";
    }
}

// Abbreviated form so it fits a 16-char LCD line alongside the "STATE: " prefix.
static const char* FSM_StateNameShort(SystemState_t state)
{
    switch (state)
    {
        case SYS_OK:            return "OK";
        case SYS_ALERT:         return "ALERT";
        case SYS_SENSOR_ERROR:  return "SENS_ERR";
        default:                return "UNKNOWN";
    }
}

void Sensors_Sample_And_Report(void)
{
    float temp_c = 0.0f, press_hpa = 0.0f;
    uint32_t adc_val = 0;

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    adc_val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    float threshold_c = Threshold_FromADC(adc_val);
    uint8_t bmp_ok = (BMP280_ReadData(&bmp280, &temp_c, &press_hpa) == HAL_OK) ? 1 : 0;

    SystemState_t prev_state = current_state;
    SystemState_t next_state = FSM_NextState(prev_state, bmp_ok, temp_c, threshold_c);

    // Edge-triggered LED alert: toggle the onboard LED exactly on the OK -> ALERT transition.
    if (prev_state == SYS_OK && next_state == SYS_ALERT)
    {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    }

    current_state = next_state;

    // --- UART report ---
    char out_str[160];
    int len;
    if (bmp_ok)
    {
        len = snprintf(out_str, sizeof(out_str),
            "Temp: %.2f C, Pressure: %.2f hPa, ADC: %lu, Threshold: %.2f C, State: %s\r\n> ",
            temp_c, press_hpa, (unsigned long)adc_val, threshold_c, FSM_StateName(current_state));
    }
    else
    {
        len = snprintf(out_str, sizeof(out_str),
            "BMP280 read error, ADC: %lu, Threshold: %.2f C, State: %s\r\n> ",
            (unsigned long)adc_val, threshold_c, FSM_StateName(current_state));
    }
    HAL_UART_Transmit(&huart2, (uint8_t*)out_str, len, 100);

    if (current_state == SYS_ALERT)
    {
        char alert_str[80];
        int alen = snprintf(alert_str, sizeof(alert_str),
            "*** ALERT: Temp %.2f C >= threshold %.2f C ***\r\n> ", temp_c, threshold_c);
        HAL_UART_Transmit(&huart2, (uint8_t*)alert_str, alen, 100);
    }

    // --- LCD report: line 1 = temp+pressure, line 2 = FSM state ---
    char lcd_line1[17], lcd_line2[17];
    if (bmp_ok)
    {
        snprintf(lcd_line1, sizeof(lcd_line1), "T:%.1fC P:%.0f", temp_c, press_hpa);
    }
    else
    {
        snprintf(lcd_line1, sizeof(lcd_line1), "BMP280 ERROR");
    }
    snprintf(lcd_line2, sizeof(lcd_line2), "STATE: %s", FSM_StateNameShort(current_state));
    lcd_print_line(0, lcd_line1);
    lcd_print_line(1, lcd_line2);
}

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
