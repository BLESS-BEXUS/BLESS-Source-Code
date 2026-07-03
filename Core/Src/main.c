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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sensors.h"
#include "telemetry.h"
#include "txhalow.h"
#include "w25qxx.h"
#include "w25log.h"
#include "minmea.h"
#include "wizchip_port.h"
#include "socket.h"
#include "elink_cmd.h"

#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Self check and initial config flags defines
#define FLAG_SENSORS_OK (1 << 0)
#define FLAG_HALOW_OK (1 << 1)
#define FLAG_FLASH_OK (1 << 2)
#define FLAG_ETHERNET_OK (1 << 3)

// GNSS VARIABLES
#define GPS_BUFFER_SIZE 100

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;

TIM_HandleTypeDef htim3;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ACQUIRE */
osThreadId_t ACQUIREHandle;
const osThreadAttr_t ACQUIRE_attributes = {
  .name = "ACQUIRE",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityHigh1,
};
/* Definitions for TXHALOW */
osThreadId_t TXHALOWHandle;
const osThreadAttr_t TXHALOW_attributes = {
  .name = "TXHALOW",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for LOGDATA */
osThreadId_t LOGDATAHandle;
const osThreadAttr_t LOGDATA_attributes = {
  .name = "LOGDATA",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for RXGNSS */
osThreadId_t RXGNSSHandle;
const osThreadAttr_t RXGNSS_attributes = {
  .name = "RXGNSS",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow6,
};
/* Definitions for RXELINK */
osThreadId_t RXELINKHandle;
const osThreadAttr_t RXELINK_attributes = {
  .name = "RXELINK",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow7,
};
/* Definitions for logDataQueue */
osMessageQueueId_t logDataQueueHandle;
const osMessageQueueAttr_t logDataQueue_attributes = {
  .name = "logDataQueue"
};
/* Definitions for txMailBox */
osMessageQueueId_t txMailBoxHandle;
const osMessageQueueAttr_t txMailBox_attributes = {
  .name = "txMailBox"
};
/* USER CODE BEGIN PV */

uint16_t count = 0;

// Initial check flags
osEventFlagsId_t InitFlags;

volatile SystemState_t current_state = STATE_INIT;

// UART VARIABLES
uint8_t rx_byte;
uint8_t rx_buffer[512];
uint16_t rx_index = 0;

TelemetryPacket rf_data_cache;

// E-LINK COMMANDS VARIABLES

volatile uint8_t new_tx_power = 9;
volatile uint8_t flag_update_tx_power = 0;

volatile uint8_t new_mcs = 0;
volatile uint8_t flag_update_mcs = 0;

// E-LINK RX variables
uint8_t eth_rx_buffer[256];

// GNSS VARIABLES
char gps_buffer[GPS_BUFFER_SIZE];
uint8_t gps_idx = 0;
uint8_t gps_rx_byte;

GPSData_t actual_gps = {0};
osMutexId_t mtxGPSHandle;

volatile uint16_t rad_pulse_count = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI3_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_SPI2_Init(void);
void StartDefaultTask(void *argument);
void StartAcquire(void *argument);
void StartTXHalow(void *argument);
void StartLogData(void *argument);
void StartRXGNSS(void *argument);
void StartRXELink(void *argument);

/* USER CODE BEGIN PFP */

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
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_SPI3_Init();
  MX_USART3_UART_Init();
  MX_SPI1_Init();
  MX_LPUART1_UART_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&hlpuart1, &gps_rx_byte, 1);

  HAL_UART_Receive_IT(&huart3, &rx_byte, 1);
  InitFlags = osEventFlagsNew(NULL);
  if(InitFlags == NULL){
	  HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_SET);
	  printf("NOT ENOUGHT RAM TO CREATE FLAGS");
	  Error_Handler();
  }


  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  mtxGPSHandle = osMutexNew(NULL);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of logDataQueue */
  logDataQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &logDataQueue_attributes);

  /* creation of txMailBox */
  txMailBoxHandle = osMessageQueueNew (1, 53, &txMailBox_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of ACQUIRE */
  ACQUIREHandle = osThreadNew(StartAcquire, NULL, &ACQUIRE_attributes);

  /* creation of TXHALOW */
  TXHALOWHandle = osThreadNew(StartTXHalow, NULL, &TXHALOW_attributes);

  /* creation of LOGDATA */
  LOGDATAHandle = osThreadNew(StartLogData, NULL, &LOGDATA_attributes);

  /* creation of RXGNSS */
  RXGNSSHandle = osThreadNew(StartRXGNSS, NULL, &RXGNSS_attributes);

  /* creation of RXELINK */
  RXELINKHandle = osThreadNew(StartRXELink, NULL, &RXELINK_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
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
  hi2c1.Init.Timing = 0x10D19CE4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 38400;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

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
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7999;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 99;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, CS_F2_Pin|GPIO_RED_LED_Pin|GPIO_YELLOW_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_ETH_GPIO_Port, CS_ETH_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_ETEMP_GPIO_Port, CS_ETEMP_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(INT_ETH_GPIO_Port, INT_ETH_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, RST_ETH_Pin|CS_F1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ITEMP_ALERT_Pin ETEMP_ALERT_Pin */
  GPIO_InitStruct.Pin = ITEMP_ALERT_Pin|ETEMP_ALERT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : HL_PWR_A_Pin */
  GPIO_InitStruct.Pin = HL_PWR_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HL_PWR_A_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_F2_Pin GPIO_RED_LED_Pin GPIO_YELLOW_LED_Pin */
  GPIO_InitStruct.Pin = CS_F2_Pin|GPIO_RED_LED_Pin|GPIO_YELLOW_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : GPIO_GREEN_LED_Pin CS_ETH_Pin */
  GPIO_InitStruct.Pin = GPIO_GREEN_LED_Pin|CS_ETH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : RD_DATA_Pin */
  GPIO_InitStruct.Pin = RD_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RD_DATA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_DMINUS_Pin USB_DPLUS_Pin */
  GPIO_InitStruct.Pin = USB_DMINUS_Pin|USB_DPLUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : CS_ETEMP_Pin */
  GPIO_InitStruct.Pin = CS_ETEMP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_ETEMP_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : INT_ETH_Pin RST_ETH_Pin CS_F1_Pin */
  GPIO_InitStruct.Pin = INT_ETH_Pin|RST_ETH_Pin|CS_F1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == RD_DATA_Pin) {
        rad_pulse_count++;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

    //============== USART3 ==============

    if (huart->Instance == USART3)
    {
        if (rx_byte == '\n' || rx_byte == '\r') {
            if (rx_index > 1) {
                rx_buffer[rx_index] = '\0';
                rx_index = 0;
                osThreadFlagsSet(TXHALOWHandle, 0x01);
            } else {
                rx_index = 0;
            }
        } else {
            if (rx_index < 510) {
                rx_buffer[rx_index] = rx_byte;
                rx_index++;
            }
        }
        HAL_UART_Receive_IT(huart, &rx_byte, 1);
    }

    //============== LPUART1 ==============

    else if (huart->Instance == LPUART1)
    {
        if (gps_idx < 99) {
            gps_buffer[gps_idx++] = (char)gps_rx_byte;
        }

        if (gps_rx_byte == '\n') {
            gps_buffer[gps_idx] = '\0';
            gps_idx = 0;

            if (RXGNSSHandle != NULL) {
                vTaskNotifyGiveFromISR((TaskHandle_t)RXGNSSHandle, NULL);
            }
        }
        HAL_UART_Receive_IT(huart, &gps_rx_byte, 1);
    }

}


void log_print_callback(uint32_t index, const uint8_t *packet_data, void *ctx) {
    printf("Paquete #%lu: ", index);
    for (int i = 0; i < W25LOG_PACKET_SIZE; i++) {
        printf("%02X ", packet_data[i]);
    }
    printf("\r\n");
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartAcquire */
/**
* @brief Function implementing the ACQUIRE thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAcquire */
void StartAcquire(void *argument)
{
  /* USER CODE BEGIN StartAcquire */
  osDelay(1000);
  Sensors_Init();

  // Heater PID variables
  float KP = 5.0;
  float KI = 2.0;
  float KD = 1.0;

  float accumulative_error = 0.0;
  float last_error = 0.0;
  float set_point = -10.0;

  float error, p, i, d;
  float output_power = 0.0;


  /** Start sensors self check**/
  HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_SET);


  if(SensorsSelfCheck()){
	  osEventFlagsSet(InitFlags, FLAG_SENSORS_OK);
  }


  osEventFlagsWait(InitFlags, FLAG_SENSORS_OK | FLAG_HALOW_OK | FLAG_FLASH_OK | FLAG_ETHERNET_OK, osFlagsWaitAll, osWaitForever);


  current_state = STATE_IDDLE;

  if(current_state == STATE_FLIGHT){
	  HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_SET);
  }

  uint32_t previousWakeTime = osKernelGetTickCount();

  /* Infinite loop */
  for(;;)
  {
	if(current_state == STATE_FLIGHT){
		HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_RESET);

		TelemetryPacket packet;
		readSensors(&packet);

		taskENTER_CRITICAL();
		packet.rad_cps = rad_pulse_count;
		rad_pulse_count = 0;
		taskEXIT_CRITICAL();

		if (osMutexAcquire(mtxGPSHandle, osWaitForever) == osOK)
		{
		    packet.gps_lat = actual_gps.lat;
		    packet.gps_lon = actual_gps.lon;
		    packet.gps_alt = actual_gps.alt;
		    packet.gps_fix_sats = actual_gps.fix_sats;
		    osMutexRelease(mtxGPSHandle);
		}

		// Add packet to the TX HALOW Mailbox
		osMessageQueuePut(txMailBoxHandle, &packet, 0, 100);
		osMessageQueuePut(logDataQueueHandle, &packet, 0, 100);

		// Adjust heater
		error = set_point - packet.int_temp;
		p = KP * error;
		accumulative_error = accumulative_error + error;
		i = KI * accumulative_error;
		d = KD * (error - last_error);

		output_power = p + i + d;
		last_error = error;

		if (output_power >= 100.0 || output_power <= 0.0){
			accumulative_error = accumulative_error - error;
		}
		output_power  = (output_power > 100.0) ? 100.0 : output_power;
		output_power  = (output_power < 0.0) ? 0.0 : output_power;

		// Update PWM with ouputpower
	}
	else if(current_state == STATE_IDDLE){
		HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_SET);
	}

	previousWakeTime += 1000;
	osDelayUntil(previousWakeTime);
  }
  /* USER CODE END StartAcquire */
}

/* USER CODE BEGIN Header_StartTXHalow */
/**
* @brief Function implementing the TXHALOW thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTXHalow */
void StartTXHalow(void *argument)
{
  /* USER CODE BEGIN StartTXHalow */
	  osDelay(5000);
	  if(SensorsSelfCheck()){
		  osEventFlagsSet(InitFlags, FLAG_HALOW_OK);
	  }

	  TelemetryPacket rx_packet; // Telemetry packet from ACQUIRE task
	  rx_index = 0;
	  memset(rx_buffer, 0, sizeof(rx_buffer));

	  memset(&rf_data_cache, 0, sizeof(TelemetryPacket));

	  uint32_t last_tx_time = 0;
	  uint32_t received_flags;

	  for(;;)
	  {
		  received_flags = osThreadFlagsWait(0x01, osFlagsWaitAny, 100);

		  if((received_flags & osFlagsError) == 0 && (received_flags & 0x01)){
			  HaLow_Extract_Data((char *)rx_buffer, &rf_data_cache);
		  }
		  if(flag_update_tx_power){
			  char str_power_tx[8];
			  snprintf(str_power_tx, sizeof(str_power_tx), "%d", new_tx_power);
			  HaLow_Send_Config_Command(&huart3, "TXPOWER", str_power_tx);
			  flag_update_tx_power = 0;
		  }
		  if(flag_update_mcs){
			  char str_mcs[8];
			  snprintf(str_mcs, sizeof(str_mcs), "%d", new_mcs);
			  HaLow_Send_Config_Command(&huart3, "TX_MCS", str_mcs);
			  flag_update_mcs = 0;
		  }
		  if(current_state == STATE_FLIGHT){
			  HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_SET);
			  HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_RESET);

			  if((HAL_GetTick() -  last_tx_time) >= DUTY_CYCLE_MS){
				  osMessageQueueGet(txMailBoxHandle, &rx_packet, NULL, 0);

				  rx_packet.rf_rssi = rf_data_cache.rf_rssi;
				  rx_packet.rf_snr = rf_data_cache.rf_snr;
			      rx_packet.rf_noise_floor = rf_data_cache.rf_noise_floor;
			      rx_packet.rf_freq_dev = rf_data_cache.rf_freq_dev;
			      rx_packet.rf_tx_mcs = rf_data_cache.rf_tx_mcs;
			      rx_packet.temp_chip = rf_data_cache.temp_chip;

				  printTelemetry(&rx_packet); // Just during tests

				  Tx_HaLow_Packet(&huart3, rx_packet);
				  last_tx_time = HAL_GetTick();
			  }
		  }
		  else if(current_state == STATE_IDDLE){
			  HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_SET);
			  HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_SET);
		  }
	  }
  /* USER CODE END StartTXHalow */
}

/* USER CODE BEGIN Header_StartLogData */
/**
* @brief Function implementing the LOGDATA thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLogData */
void StartLogData(void *argument)
{
  /* USER CODE BEGIN StartLogData */
	 W25QXX_ID_t flash_id;
	 W25LOG_State_t log_state;
	 TelemetryPacket packet_to_log;

	 osDelay(2000);

	 uint8_t flash_present = W25QXX_Init(&flash_id);
	 uint8_t log_ready = 0;

	  if (flash_present) {
	      log_ready = W25LOG_Init(&log_state);
	  } else {
	      printf("W25QXX NO RESPONDE\r\n");
	  }

	  if (flash_present && log_ready) {
	      osEventFlagsSet(InitFlags, FLAG_FLASH_OK);
	      printf("Flash Ready. Storaged Packets: %lu\r\n", log_state.packet_count);
	  } else {
	      printf("Flash Init Error \r\n");
	  }
	  W25LOG_Format(&log_state);

	  W25LOG_DumpToUART(&huart2, &log_state);

	  /* Infinite loop */
	  for(;;)
	  {
		if(current_state == STATE_FLIGHT){
			if (osMessageQueueGet(logDataQueueHandle, &packet_to_log, NULL, osWaitForever) == osOK) {
				if (flash_present && log_ready) {
					if (W25LOG_WritePacket(&log_state, (uint8_t*)&packet_to_log) != 0) {
						printf("Full Flash\r\n");
					}
				}
			}
		}else{
			osDelay(50);
		}

	  }
  /* USER CODE END StartLogData */
}

/* USER CODE BEGIN Header_StartRXGNSS */
/**
* @brief Function implementing the RXGNSS thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRXGNSS */
void StartRXGNSS(void *argument)
{
  /* USER CODE BEGIN StartRXGNSS */
  /* Infinite loop */
	for(;;)
	  {
	    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	    switch (minmea_sentence_id(gps_buffer, false))
	    {
	      case MINMEA_SENTENCE_GGA:
	      {
	        struct minmea_sentence_gga gga;
	        if (minmea_parse_gga(&gga, gps_buffer))
	        {
	        	float lat_decimal = minmea_tocoord(&gga.latitude);
	        	float lon_decimal = minmea_tocoord(&gga.longitude);

	        	int32_t lat_fixed = (int32_t)(lat_decimal * 10000000.0f);
	        	int32_t lon_fixed = (int32_t)(lon_decimal * 10000000.0f);
	        	uint16_t alt = (uint16_t)minmea_tofloat(&gga.altitude);

	        	uint8_t fix = (gga.fix_quality > 3) ? 3 : gga.fix_quality;
	        	uint8_t sats = (gga.satellites_tracked > 63) ? 63 : gga.satellites_tracked;
	        	int8_t fix_sats_packed = (int8_t)((fix << 6) | sats);

	        	if (osMutexAcquire(mtxGPSHandle, osWaitForever) == osOK)
	        	{
	        	    actual_gps.lat = lat_fixed;
	        	    actual_gps.lon = lon_fixed;
	        	    actual_gps.alt = alt;
	        	    actual_gps.fix_sats = fix_sats_packed;
	        	    osMutexRelease(mtxGPSHandle);
	        	}

	        }
	        break;
	      }

	      case MINMEA_SENTENCE_RMC:
	      {
	        struct minmea_sentence_rmc rmc;
	        if (minmea_parse_rmc(&rmc, gps_buffer)) {

	        }
	        break;
	      }

	      default:
	        break;
	    }
	  }
  /* USER CODE END StartRXGNSS */
}

/* USER CODE BEGIN Header_StartRXELink */
/**
* @brief Function implementing the RXELINK thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRXELink */
void StartRXELink(void *argument)
{
  /* USER CODE BEGIN StartRXELink */
	if(W5500_Init() == 1){
	      osEventFlagsSet(InitFlags, FLAG_ETHERNET_OK);
	  }

	  if(socket(0, Sn_MR_UDP, UDP_PORT, 0) != 0) {
	      printf("Error while opening UDP port for RX\r\n");
	  }

	  /* Infinite loop */
	  for(;;)
	  {
	      uint16_t data_len = getSn_RX_RSR(0);

	      if(data_len > 0)
	      {
	          uint8_t target_ip[4];
	          uint16_t target_port;

	          int32_t len = recvfrom(0, eth_rx_buffer, data_len, target_ip, &target_port);

	          if(len == sizeof(ELinkCommand)) {

	              // Map from raw to struct
	              ELinkCommand* incoming_cmd = (ELinkCommand*)eth_rx_buffer;
	              uint8_t result = RECEIVE_ELINK_CMD(incoming_cmd);

	              // Callback to the GS
	              switch(result) {
	                  case 0:
	                      sendto(0, (uint8_t*)"ACK: CMD EXECTUTED\r\n", 19, target_ip, target_port);
	                      printf("Command %d processed.\r\n", incoming_cmd->cmd_id);
	                      break;
	                  case 1:
	                      sendto(0, (uint8_t*)"ERR: SYNC WORD\r\n", 16, target_ip, target_port);
	                      printf("Sync error.\r\n");
	                      break;
	                  case 2:
	                      sendto(0, (uint8_t*)"ERR: CHECKSUM\r\n", 15, target_ip, target_port);
	                      printf("Checksum Error.\r\n");
	                      break;
	                  case 3:
	                      sendto(0, (uint8_t*)"ERR: UNKNOWN ID\r\n", 17, target_ip, target_port);
	                      printf("Command %d doesn't exist.\r\n", incoming_cmd->cmd_id);
	                      break;
	              }
	          }
	          else {
	              printf("Wrong Size (%ld bytes, expected %d).\r\n", len, sizeof(ELinkCommand));
	          }
	      }

	      osDelay(50);
	  }
  /* USER CODE END StartRXELink */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM7 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM7)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
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
