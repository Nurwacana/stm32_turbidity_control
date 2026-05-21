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
#include "cmsis_os.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c_lcd.h"
#include <stdio.h>
#include "math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define BUFFER_CONVERSION_SIZE 1500
typedef struct {
	uint16_t buffer[BUFFER_CONVERSION_SIZE];
	uint16_t buffer_counter;
	uint32_t buffer_total;
	float volt;
	float volt_mod;
	float value;
	uint32_t count;
}AdcTypedef_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

osThreadId defaultTaskHandle;
osThreadId usbTaskHandle;
osThreadId turbidityTaskHandle;
/* USER CODE BEGIN PV */

HAL_StatusTypeDef I2c_lcd;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
void StartDefaultTask(void const * argument);
void StartUsbTask(void const * argument);
void StartTurbidityTask(void const * argument);

/* USER CODE BEGIN PFP */
void revive_I2C_rtos(I2C_HandleTypeDef *hi2c,
		GPIO_TypeDef *GPIOx,
		uint16_t scl_pin,
		uint16_t sda_pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* 1. Abort & DeInit I2C + DMA */
	HAL_I2C_DeInit(hi2c);

	if (hi2c->hdmarx)
	{
		HAL_DMA_Abort(hi2c->hdmarx);
		HAL_DMA_DeInit(hi2c->hdmarx);
	}

	/* 2. Enable GPIO clock (manual, user must ensure correct RCC) */
	/* NOTE: Pastikan RCC GPIOx sudah di-enable sebelum panggil fungsi ini */

	/* 3. Set SDA & SCL sebagai GPIO Open-Drain */
	GPIO_InitStruct.Pin   = scl_pin | sda_pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);

	/* 4. Force SDA & SCL HIGH */
	HAL_GPIO_WritePin(GPIOx, scl_pin | sda_pin, GPIO_PIN_SET);
	osDelay(1);

	/* 5. Clock pulse SCL untuk release slave (9 pulse I2C spec) */
	for (uint8_t i = 0; i < 9; i++)
	{
		HAL_GPIO_WritePin(GPIOx, scl_pin, GPIO_PIN_RESET);
		osDelay(1);
		HAL_GPIO_WritePin(GPIOx, scl_pin, GPIO_PIN_SET);
		osDelay(1);
	}

	/* 6. Pastikan SDA released */
	HAL_GPIO_WritePin(GPIOx, sda_pin, GPIO_PIN_SET);
	osDelay(1);

	/* 7. Kembalikan pin ke AF Open-Drain (I2C) */
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);

	/* 8. Re-init I2C */
	HAL_I2C_Init(hi2c);
}
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
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

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
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 1024);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of usbTask */
  osThreadDef(usbTask, StartUsbTask, osPriorityNormal, 0, 128);
  usbTaskHandle = osThreadCreate(osThread(usbTask), NULL);

  /* definition and creation of turbidityTask */
  osThreadDef(turbidityTask, StartTurbidityTask, osPriorityIdle, 0, 256);
  turbidityTaskHandle = osThreadCreate(osThread(turbidityTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
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
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(D_MOTOR_GPIO_Port, D_MOTOR_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : D_MOTOR_Pin */
  GPIO_InitStruct.Pin = D_MOTOR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(D_MOTOR_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
I2C_LCD_HandleTypeDef g_lcd = {
		.hi2c = &hi2c1,
		.address = 0x4E
};

#define SLAVE_ADDRESS_LCD 0x4E

#define LCD_BACKLIGHT 0x08
#define ENABLE 0x04

#define ADC_TO_VOLT (1.0F/4095.0F) * 3.3F

char buffer[16];

uint32_t count;

volatile uint16_t adc_buffer[1];

AdcTypedef_t g_turbidity;

_Bool motor_state = 0;
_Bool latch_condition = 0;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	if (hadc->Instance == ADC1)
	{
		// data baru siap
		g_turbidity.buffer_total -= g_turbidity.buffer[g_turbidity.buffer_counter];

		g_turbidity.buffer[g_turbidity.buffer_counter] = adc_buffer[0];

		g_turbidity.buffer_total += g_turbidity.buffer[g_turbidity.buffer_counter];

		g_turbidity.buffer_counter++;

		if(g_turbidity.buffer_counter >= (BUFFER_CONVERSION_SIZE)) {
			g_turbidity.buffer_counter = 0;
		}

	}
}


/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 5 */
	lcd_init(&g_lcd);
	lcd_clear(&g_lcd);
	/* Infinite loop */
	for(;;)
	{
		lcd_gotoxy(&g_lcd, 0, 0);
		sprintf(buffer, "KEKERUHAN:%4.2f   ", g_turbidity.value);
		lcd_puts(&g_lcd, buffer);

		lcd_gotoxy(&g_lcd, 0, 1);

		if(motor_state) {
			sprintf(buffer, "MOTOR: ON  ");
		} else {
			sprintf(buffer, "MOTOR: OFF ");
		}
		lcd_puts(&g_lcd, buffer);

		osDelay(100);
	}
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartUsbTask */
/**
 * @brief Function implementing the usbTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartUsbTask */
void StartUsbTask(void const * argument)
{
  /* USER CODE BEGIN StartUsbTask */
	/* Infinite loop */
	for(;;)
	{
		osDelay(1);
	}
  /* USER CODE END StartUsbTask */
}

/* USER CODE BEGIN Header_StartTurbidityTask */
/**
 * @brief Function implementing the turbidityTask thread.
 * @param argument: Not used
 * @retval None
 */

/* USER CODE END Header_StartTurbidityTask */
void StartTurbidityTask(void const * argument)
{
  /* USER CODE BEGIN StartTurbidityTask */
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 1);
	const float multiplier_conversion = 1.0F / BUFFER_CONVERSION_SIZE;
	/* Infinite loop */
	for(;;)
	{
		g_turbidity.volt = (float)g_turbidity.buffer_total * multiplier_conversion * ADC_TO_VOLT;

		g_turbidity.volt_mod = g_turbidity.volt * (1.7F/3.3F) + 2.5;
//		g_turbidity.value = (-1120.4F * powf(g_turbidity.volt_mod, 2))
//				+ (5742.3F * g_turbidity.volt_mod)
//				- 4352.9F;

		g_turbidity.value = (1- (g_turbidity.volt /3.3)) * 100.0F;

		if(g_turbidity.value < 0) g_turbidity.value = 0;

		if(g_turbidity.value < 15) {
			motor_state = 0;
		} else if (g_turbidity.value > 50) {
			motor_state = 1;
		}

		HAL_GPIO_WritePin(D_MOTOR_GPIO_Port, D_MOTOR_Pin, !motor_state);

		osDelay(10);
	}
  /* USER CODE END StartTurbidityTask */
}

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
