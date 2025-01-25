/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "i2c.h"
#include "temperature_sensor.h"
#include "lcd.h"
#include "font.h"

#include <string.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define TMP117_ADDRESS         0x48 << 1  // Adres 7-bitowy przesunięty w lewo o 1 bit
#define TMP117_TEMP_REG        0x00       // Rejestr temperatury

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

float TMP117_ReadTemperature(void) {
    uint8_t temp_data[2];
    int16_t raw_temp;
    float temperature;

    // Odczyt 2 bajtów z rejestru temperatury
    if (HAL_I2C_Mem_Read(&hi2c1, TMP117_ADDRESS, TMP117_TEMP_REG, I2C_MEMADD_SIZE_8BIT, temp_data, 2, HAL_MAX_DELAY) == HAL_OK) {
        // �?ączenie odczytanych bajtów w wartość 16-bitową
        raw_temp = (int16_t)((temp_data[0] << 8) | temp_data[1]);

        // Przeliczenie na temperaturę w stopniach Celsjusza
        temperature = raw_temp * 0.0078125;
    } else {
        // W przypadku błędu zwróć wartość specjalną
        temperature = -273.15; // Wartość wskazująca na błąd
    }

    return temperature;
}

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

float temperature = 0.0f;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	temperature_sensor_init();

	  char min_pulse_tab[22];
	  float temp = 0.0;

	  lcd_init();
	  osDelay(100);

  /* Infinite loop */
  for(;;)
  {
	  temp = temperature_sensor_get_temperature();
	  sprintf(min_pulse_tab, "temperature: %f", temp);
	  fill_with(BLACK);
	  LCD_DisplayString(5, 5, min_pulse_tab, WHITE, LCD_FONT8);
	  lcd_copy();

      osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */



/* USER CODE END Application */

