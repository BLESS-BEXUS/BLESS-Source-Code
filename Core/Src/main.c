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
#include "gnss_i2c.h"
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

/* IWDG timeout = (reload + 1) * prescaler / LSI ~= 4 s at 32 kHz.
 * The margin accommodates LSI tolerance while ACQUIRE runs every second. */
#define BLESS_IWDG_PRESCALER (IWDG_PR_PR_2 | IWDG_PR_PR_0) /* /128 */
#define BLESS_IWDG_RELOAD    999U
#define BLESS_IWDG_INIT_TIMEOUT_MS 100U
#define BLESS_IWDG_KEY_RELOAD      0x0000AAAAU
#define BLESS_IWDG_KEY_ENABLE      0x0000CCCCU
#define BLESS_IWDG_KEY_WRITE       0x00005555U

/* Set to 1 only for destructive development/cold-test builds. */
#ifndef BLESS_DEV_FORMAT_FLASH_ON_BOOT
#define BLESS_DEV_FORMAT_FLASH_ON_BOOT 0
#endif
#define BLESS_BOOT_RECORD_COUNT UINT32_MAX

// GNSS VARIABLES
#define GPS_BUFFER_SIZE 100
#define GPS_I2C_READ_BUFFER_SIZE 128
#define GPS_I2C_POLL_PERIOD_MS 20

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
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for TXELINK */
osThreadId_t TXELINKHandle;
const osThreadAttr_t TXELINK_attributes = {
  .name = "TXELINK",
  .stack_size = 512 * 4,
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
/* Definitions for txElinkMailBox */
osMessageQueueId_t txElinkMailBoxHandle;
const osMessageQueueAttr_t txElinkMailBox_attributes = {
  .name = "txElinkMailBox"
};
/* USER CODE BEGIN PV */

uint16_t count = 0;

// Initial check flags
osEventFlagsId_t InitFlags;

volatile SystemState_t current_state = STATE_INIT;


// UART VARIABLES
uint8_t rx_byte;
char halow_rx_lines[HALOW_RX_LINE_DEPTH][HALOW_RX_LINE_SIZE];
volatile uint8_t halow_rx_line_head = 0U;
volatile uint8_t halow_rx_line_tail = 0U;
volatile uint8_t halow_rx_line_count = 0U;
volatile uint16_t rx_index = 0U;

TelemetryPacket rf_data_cache;

// E-LINK COMMANDS VARIABLES

volatile uint8_t new_tx_power = 9;
volatile uint8_t flag_update_tx_power = 0;

volatile uint8_t new_mcs = 0;
volatile uint8_t flag_update_mcs = 0;

volatile uint8_t flag_dump_spi_data = 0;
volatile uint8_t flag_save_data = 0U;

volatile uint8_t heater_manual_enabled = 0U;
volatile uint8_t heater_manual_power_pct = 0U;

volatile uint8_t flag_halow_connect = 0U;
volatile uint8_t halow_rf_silenced = 0U;
volatile uint8_t flag_force_halow_tx = 0U;

// E-LINK RX variables
uint8_t eth_rx_buffer[256];

// GNSS VARIABLES
char gps_buffer[GPS_BUFFER_SIZE];
uint8_t gps_idx = 0;

GPSData_t actual_gps = {0};
osMutexId_t mtxGPSHandle;
osMutexId_t mtxI2C1Handle;

volatile uint16_t rad_pulse_count = 0;
uint8_t boot_reset_cause = 0U;



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
void StartTXELINK(void *argument);

/* USER CODE BEGIN PFP */

static void GNSS_ProcessSentence(const char *sentence);
static void GNSS_FeedByte(uint8_t byte);
static void ELink_QueueNewestTelemetry(const TelemetryPacket *packet);
static void ELink_EnsureTxSocket(void);
static uint8_t BLESS_IWDG_Init(void);
static void BLESS_IWDG_Refresh(void);

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
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET) {
      boot_reset_cause = 1U;
  } else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != RESET) {
      boot_reset_cause = 2U;
  } else if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) != RESET) {
      boot_reset_cause = 3U;
  } else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET) {
      boot_reset_cause = 4U;
  }
  __HAL_RCC_CLEAR_RESET_FLAGS();

  /* Reset forzoso de periféricos tras posible evento de tensión */
    __HAL_RCC_SPI1_FORCE_RESET();
    __HAL_RCC_SPI1_RELEASE_RESET();
    __HAL_RCC_SPI2_FORCE_RESET();
    __HAL_RCC_SPI2_RELEASE_RESET();
    __HAL_RCC_SPI3_FORCE_RESET();
    __HAL_RCC_SPI3_RELEASE_RESET();

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
  if(BLESS_IWDG_Init() == 0U){
	  Error_Handler();
  }
  InitFlags = osEventFlagsNew(NULL);
  if(InitFlags == NULL){
	  HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_SET);
	  printf("NOT ENOUGHT RAM TO CREATE FLAGS");
	  Error_Handler();
  }

  /* WAIT COUNTER TO SEE INIT*/


  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  mtxGPSHandle = osMutexNew(NULL);
  mtxI2C1Handle = osMutexNew(NULL);
  if ((mtxGPSHandle == NULL) || (mtxI2C1Handle == NULL)) {
      Error_Handler();
  }
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of logDataQueue */
  logDataQueueHandle = osMessageQueueNew (16, 53, &logDataQueue_attributes);

  /* creation of txMailBox */
  txMailBoxHandle = osMessageQueueNew (1, 53, &txMailBox_attributes);

  /* creation of txElinkMailBox */
  txElinkMailBoxHandle = osMessageQueueNew (1, 53, &txElinkMailBox_attributes);

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

  /* creation of TXELINK */
  TXELINKHandle = osThreadNew(StartTXELINK, NULL, &TXELINK_attributes);

  /* Start HaLow UART reception only after TXHALOWHandle exists. */
  HAL_UART_Receive_IT(&huart3, &rx_byte, 1);

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
  /* Conservative operational clock for the flat-cable SPI link. */
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
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

  /*Configure GPIO pin : GPIO_GREEN_LED_Pin */
  GPIO_InitStruct.Pin = GPIO_GREEN_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIO_GREEN_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CS_ETH_Pin */
  GPIO_InitStruct.Pin = CS_ETH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_ETH_GPIO_Port, &GPIO_InitStruct);

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

  /* W5500 INTn is unused: leave the MCU pin in high impedance. */
  GPIO_InitStruct.Pin = INT_ETH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(INT_ETH_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RST_ETH_Pin CS_F1_Pin */
  GPIO_InitStruct.Pin = RST_ETH_Pin|CS_F1_Pin;
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

static uint8_t BLESS_IWDG_Init(void)
{
    uint32_t start = HAL_GetTick();

#ifdef DEBUG
    /* Keep breakpoints usable in development builds. The watchdog continues
     * running normally in Release/mission builds. */
    __HAL_DBGMCU_FREEZE_IWDG();
#endif

    /* Starting the IWDG forces its independent LSI clock on. */
    WRITE_REG(IWDG->KR, BLESS_IWDG_KEY_ENABLE);
    WRITE_REG(IWDG->KR, BLESS_IWDG_KEY_WRITE);
    WRITE_REG(IWDG->PR, BLESS_IWDG_PRESCALER);
    WRITE_REG(IWDG->RLR, BLESS_IWDG_RELOAD);

    while ((IWDG->SR & (IWDG_SR_WVU | IWDG_SR_RVU | IWDG_SR_PVU)) != 0U) {
        if ((HAL_GetTick() - start) > BLESS_IWDG_INIT_TIMEOUT_MS) {
            return 0U;
        }
    }

    /* Disable window mode and load the configured reload value. */
    WRITE_REG(IWDG->WINR, IWDG_WINR_WIN);
    WRITE_REG(IWDG->KR, BLESS_IWDG_KEY_RELOAD);
    return 1U;
}

static void BLESS_IWDG_Refresh(void)
{
    WRITE_REG(IWDG->KR, BLESS_IWDG_KEY_RELOAD);
}

static void ELink_QueueNewestTelemetry(const TelemetryPacket *packet)
{
    TelemetryPacket stale_packet;

    if (osMessageQueuePut(txElinkMailBoxHandle, packet, 0U, 0U) == osOK) {
        return;
    }

    /* Keep the newest 1 Hz sample while the E-Link destination is offline. */
    (void)osMessageQueueGet(txElinkMailBoxHandle, &stale_packet, NULL, 0U);
    if (osMessageQueuePut(txElinkMailBoxHandle, packet, 0U, 0U) != osOK) {
        printf("E-Link telemetry mailbox replacement failed.\r\n");
    }
}

static void ELink_EnsureTxSocket(void)
{
    while (getSn_SR(1) != SOCK_UDP) {
        close(1);
        if (socket(1, Sn_MR_UDP, 5001, 0) == 1) {
            printf("E-Link TX socket ready on local port 5001.\r\n");
            return;
        }

        printf("E-Link TX socket unavailable; retrying.\r\n");
        osDelay(250);
    }
}

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
                halow_rx_lines[halow_rx_line_head][rx_index] = '\0';
                if (halow_rx_line_count < HALOW_RX_LINE_DEPTH) {
                    halow_rx_line_head = (uint8_t)((halow_rx_line_head + 1U)
                        % HALOW_RX_LINE_DEPTH);
                    halow_rx_line_count++;
                }
                rx_index = 0;
                if (TXHALOWHandle != NULL) {
                    osThreadFlagsSet(TXHALOWHandle, 0x01);
                }
            } else {
                rx_index = 0;
            }
        } else {
            if (halow_rx_line_count < HALOW_RX_LINE_DEPTH
                    && rx_index < (HALOW_RX_LINE_SIZE - 1U)) {
                halow_rx_lines[halow_rx_line_head][rx_index] = (char)rx_byte;
                rx_index++;
            }
        }
        HAL_UART_Receive_IT(huart, &rx_byte, 1);
    }

}

static void GNSS_ProcessSentence(const char *sentence)
{
    switch (minmea_sentence_id(sentence, false))
    {
      case MINMEA_SENTENCE_GGA:
      {
        struct minmea_sentence_gga gga;
        if (minmea_parse_gga(&gga, sentence))
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
        if (minmea_parse_rmc(&rmc, sentence)) {
            /* Reserved for speed, course and UTC date/time. */
        }
        break;
      }

      default:
        break;
    }
}

static void GNSS_FeedByte(uint8_t byte)
{
    if (byte == '$') {
        gps_idx = 0;
    }

    /* Ignore padding and partial data until the beginning of a NMEA sentence. */
    if ((gps_idx == 0U) && (byte != '$')) {
        return;
    }

    if (byte == '\r') {
        return;
    }

    if (byte == '\n') {
        gps_buffer[gps_idx] = '\0';
        GNSS_ProcessSentence(gps_buffer);
        gps_idx = 0;
        return;
    }

    if (gps_idx < (GPS_BUFFER_SIZE - 1U)) {
        gps_buffer[gps_idx++] = (char)byte;
    } else {
        /* Drop an oversized/corrupt sentence and resynchronize at the next '$'. */
        gps_idx = 0;
    }
}


void log_print_callback(uint32_t index, const uint8_t *packet_data, void *ctx) {
    printf("Packet #%lu: ", index);
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
  /* ACQUIRE is the sole owner of the watchdog refresh. */
  BLESS_IWDG_Refresh();
  osDelay(1000);
  Sensors_Init();

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

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
  BLESS_IWDG_Refresh();


  //osEventFlagsWait(InitFlags, FLAG_SENSORS_OK | FLAG_HALOW_OK | FLAG_FLASH_OK | FLAG_ETHERNET_OK, osFlagsWaitAll, osWaitForever);


  //current_state = STATE_IDDLE;
  current_state = STATE_FLIGHT;


  if(current_state == STATE_FLIGHT){
	  HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_SET);
  }

  uint32_t packet_counter = 0;
  uint32_t previousWakeTime = osKernelGetTickCount();

  /* Infinite loop */
  for(;;)
  {
		TelemetryPacket packet = {0};
		SystemState_t packet_state;

		taskENTER_CRITICAL();
		packet_state = current_state;
		taskEXIT_CRITICAL();

	if(packet_state == STATE_FLIGHT){
		HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_RESET);
	}
	else{
		HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_SET);
	}

	packet.count = packet_counter;
	packet.timestamp = HAL_GetTick();
	packet_counter++;

	readSensors(&packet);

	/* Snapshot the latest HaLow statistics before distributing the packet.
	 * This makes flash, HaLow and E-Link receive the same complete data. */
	taskENTER_CRITICAL();
	packet.rf_rssi = rf_data_cache.rf_rssi;
	packet.rf_snr = rf_data_cache.rf_snr;
	packet.rf_freq_dev = rf_data_cache.rf_freq_dev;
	packet.rf_success_rate = rf_data_cache.rf_success_rate;
	packet.rf_noise_floor = rf_data_cache.rf_noise_floor;
	packet.rf_tx_mcs = rf_data_cache.rf_tx_mcs;
	packet.temp_chip = rf_data_cache.temp_chip;
	packet.system_state = (uint16_t)packet_state;
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

	if(packet_state == STATE_FLIGHT){
		uint8_t manual_enabled;
		uint8_t manual_power;

		taskENTER_CRITICAL();
		manual_enabled = heater_manual_enabled;
		manual_power = heater_manual_power_pct;
		taskEXIT_CRITICAL();

		if(manual_enabled != 0U){
			output_power = (float)manual_power;
			accumulative_error = 0.0f;
			last_error = 0.0f;
		}else{
			/* Autonomous PID control. */
			error = set_point - (packet.int_temp / 100.0f);
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
		}

		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, (uint32_t)output_power);
	}else{
		output_power = 0.0f;
		accumulative_error = 0.0f;
		last_error = 0.0f;
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0U);
	}

	printf("Heater Power: %.2f", output_power);
	packet.heater_power = (uint16_t)(output_power * 100.0f);
	Telemetry_Finalize(&packet);
	Telemetry_UpdateLatest(&packet);

	/* E-Link is the command/status channel and remains alive in every state. */
	ELink_QueueNewestTelemetry(&packet);

	if(packet_state == STATE_FLIGHT){
		osMessageQueuePut(txMailBoxHandle, &packet, 0, 100);
		osMessageQueuePut(logDataQueueHandle, &packet, 0, 100);
	}

	previousWakeTime += 1000;
	BLESS_IWDG_Refresh();
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
	  if(Config_HaLow(&huart3)){
		  osEventFlagsSet(InitFlags, FLAG_HALOW_OK);
	  }

	  TelemetryPacket rx_packet; // Telemetry packet from ACQUIRE task
	  rx_index = 0;
	  memset(halow_rx_lines, 0, sizeof(halow_rx_lines));

	  memset(&rf_data_cache, 0, sizeof(TelemetryPacket));

	  uint32_t last_tx_time = 0;
	  uint32_t received_flags;

	  for(;;)
	  {
		  received_flags = osThreadFlagsWait(0x01, osFlagsWaitAny, 100);

		  if((received_flags & osFlagsError) == 0 && (received_flags & 0x01)){
			  char line[HALOW_RX_LINE_SIZE];
			  while(HaLow_PopRxLine(line, sizeof(line))){
				  HaLow_Extract_Data(line, &rf_data_cache);
			  }
		  }
		  if(flag_halow_connect){
			  flag_halow_connect = 0U;
			  if(Config_HaLow(&huart3)){
				  osEventFlagsSet(InitFlags, FLAG_HALOW_OK);
			  }
		  }
		  if(flag_update_tx_power && (halow_rf_silenced == 0U)){
			  char str_power_tx[8];
			  snprintf(str_power_tx, sizeof(str_power_tx), "%d", new_tx_power);
			  (void)HaLow_Send_Config_Command(&huart3, "TXPOWER", str_power_tx);
			  flag_update_tx_power = 0;
		  }
		  if(flag_update_mcs && (halow_rf_silenced == 0U)){
			  char str_mcs[8];
			  snprintf(str_mcs, sizeof(str_mcs), "%d", new_mcs);
			  (void)HaLow_Send_Config_Command(&huart3, "TX_MCS", str_mcs);
			  flag_update_mcs = 0;
		  }
		  if(flag_force_halow_tx && (halow_rf_silenced == 0U)){
			  TelemetryPacket forced_packet;
			  flag_force_halow_tx = 0U;
			  if(Telemetry_GetLatest(&forced_packet)){
				  Tx_HaLow_Packet(&huart3, forced_packet);
				  last_tx_time = HAL_GetTick();
			  }
		  }
		  if((current_state == STATE_FLIGHT) && (halow_rf_silenced == 0U)){
			  HAL_GPIO_WritePin(GPIO_GREEN_LED_GPIO_Port, GPIO_GREEN_LED_Pin, GPIO_PIN_SET);
			  HAL_GPIO_WritePin(GPIO_YELLOW_LED_GPIO_Port, GPIO_YELLOW_LED_Pin, GPIO_PIN_RESET);

			  if((HAL_GetTick() -  last_tx_time) >= DUTY_CYCLE_MS){
				  if(osMessageQueueGet(txMailBoxHandle, &rx_packet, NULL, 0) == osOK){
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
	      log_state.reset_cause = boot_reset_cause;
	  } else {
	      printf("W25QXX NO RESPONDE\r\n");
	  }

	  #if BLESS_DEV_FORMAT_FLASH_ON_BOOT
	  if (flash_present && log_ready && W25LOG_Format(&log_state) == 0U) {
	      log_ready = 0U;
	      printf("Flash format failed\r\n");
	  }
	  #endif

	  if (flash_present && log_ready) {
	      /* Persist the boot even if the system resets before its first 1 Hz
	       * acquisition. UINT32_MAX distinguishes this marker from samples. */
	      TelemetryPacket boot_record = {0};
	      boot_record.timestamp = HAL_GetTick();
	      boot_record.count = BLESS_BOOT_RECORD_COUNT;
	      boot_record.system_state = (uint16_t)STATE_INIT;
	      Telemetry_Finalize(&boot_record);
	      if (W25LOG_WritePacket(&log_state, (uint8_t *)&boot_record) != 0U) {
	          log_ready = 0U;
	          printf("Flash boot record failed\r\n");
	      }
	  }

	  if (flash_present && log_ready) {
	      osEventFlagsSet(InitFlags, FLAG_FLASH_OK);
	      printf("Flash Ready. Stored Packets: %lu, Boot ID: %lu, Reset: %u\r\n",
	             log_state.packet_count, log_state.boot_id,
	             log_state.reset_cause);
	  } else {
	      printf("Flash Init Error \r\n");
	  }

	  /* Infinite loop */
	  for(;;)
	  {
		if (osMessageQueueGet(logDataQueueHandle, &packet_to_log, NULL, 100) == osOK
				&& flash_present && log_ready) {
			uint8_t write_status = W25LOG_WritePacket(
					&log_state, (uint8_t*)&packet_to_log);
			if (write_status != 0U) {
				printf("Flash log write failed (%u)\r\n", write_status);
			}
		}

		if(flag_save_data){
			/* Writes are synchronous. Draining the queue guarantees every sample
			 * already produced before this command is committed to flash. */
			while(osMessageQueueGet(logDataQueueHandle, &packet_to_log,
					NULL, 0U) == osOK){
				if(flash_present && log_ready
						&& W25LOG_WritePacket(&log_state,
								(uint8_t*)&packet_to_log) != 0U){
					printf("SAVE_DATA flash write failed\r\n");
					break;
				}
			}
			flag_save_data = 0U;
		}

		if(flag_dump_spi_data && flash_present && log_ready){
			W25LOG_DumpToUART(&huart2, &log_state);
			flag_dump_spi_data = 0;
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
  uint8_t i2c_data[GPS_I2C_READ_BUFFER_SIZE];
  uint16_t bytes_read;
  HAL_StatusTypeDef status;

  /* Wait until the MAX-M10S acknowledges its default I2C address. */
  do {
      status = HAL_ERROR;
      if (osMutexAcquire(mtxI2C1Handle, osWaitForever) == osOK) {
          status = GNSS_I2C_IsReady(&hi2c1);
          osMutexRelease(mtxI2C1Handle);
      }
      if (status != HAL_OK) {
          osDelay(250);
      }
  } while (status != HAL_OK);

  /* Infinite loop */
  for (;;)
  {
      bytes_read = 0U;
      status = HAL_ERROR;

      if (osMutexAcquire(mtxI2C1Handle, osWaitForever) == osOK) {
          status = GNSS_I2C_Read(&hi2c1,i2c_data, sizeof(i2c_data), &bytes_read);
          osMutexRelease(mtxI2C1Handle);
      }

      if (status == HAL_OK) {
          for (uint16_t i = 0U; i < bytes_read; i++) {
              GNSS_FeedByte(i2c_data[i]);
          }
          osDelay(GPS_I2C_POLL_PERIOD_MS);
      } else {
          /* Avoid flooding a faulty or disconnected bus. */
          osDelay(100);
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
	_Static_assert(sizeof(ELinkCommand) == 10U, "Unexpected E-Link command size");

	/* Allow the W5500 supply and 25 MHz oscillator to settle after power-up. */
	printf("Waiting 2 seconds before W5500 initialization...\r\n");
	osDelay(2000);

	while (W5500_Init() != 1) {
		printf("W5500 initialization failed; retrying in 1 second\r\n");
		osDelay(1000);
	}
	osEventFlagsSet(InitFlags, FLAG_ETHERNET_OK);

	while (socket(0, Sn_MR_UDP, UDP_PORT, 0) != 0) {
		printf("Error while opening UDP port for RX; retrying\r\n");
		osDelay(1000);
	}
	printf("E-Link UDP RX ready on W5500 port %u (socket 0).\r\n", UDP_PORT);

	  /* Infinite loop */
	  for(;;)
	  {
		  if (getSn_SR(0) != SOCK_UDP) {
			  printf("E-Link RX socket left UDP state; reopening port %u.\r\n",
			         UDP_PORT);
			  close(0);
			  while (socket(0, Sn_MR_UDP, UDP_PORT, 0) != 0) {
				  osDelay(100);
			  }
		  }

		  uint16_t data_len = getSn_RX_RSR(0);

		      if(data_len > 0)
		      {
		          uint8_t target_ip[4];
		          uint16_t target_port;
		          uint16_t receive_capacity = (uint16_t)sizeof(eth_rx_buffer);
		          if (data_len < receive_capacity) {
		              receive_capacity = data_len;
		          }

		          int32_t len = recvfrom(0, eth_rx_buffer, receive_capacity,
		                                 target_ip, &target_port);
		          printf("E-Link UDP RX: %ld bytes from %u.%u.%u.%u:%u.\r\n",
		                 (long)len,
		                 target_ip[0], target_ip[1], target_ip[2], target_ip[3],
		                 target_port);

		          if(len == sizeof(ELinkCommand)) {
		              ELinkCommand incoming_cmd;
		              memcpy(&incoming_cmd, eth_rx_buffer, sizeof(incoming_cmd));
		              ELinkCommandResult result = RECEIVE_ELINK_CMD(&incoming_cmd);

		              // Callback to the GS
		              switch(result) {
		                  case ELINK_CMD_OK:
		                      if(incoming_cmd.cmd_id == ELINK_CMD_PING) {
		                          ELinkCommand ping_response;
		                          Build_Ping_Response(&incoming_cmd, &ping_response);
		                          sendto(0, (uint8_t*)&ping_response, sizeof(ping_response), target_ip, target_port);
		                          uint16_t ping_token = (uint16_t)incoming_cmd.payload[0]
		                                                | ((uint16_t)incoming_cmd.payload[1] << 8);
		                          printf("Ping 0x%04X responded to %u.%u.%u.%u:%u.\r\n",
		                                 ping_token,
		                                 target_ip[0], target_ip[1], target_ip[2], target_ip[3],
		                                 target_port);
		                      } else {
		                          sendto(0, (uint8_t*)"ACK: CMD EXECTUTED\r\n", 19, target_ip, target_port);
		                          printf("Command %d processed.\r\n", incoming_cmd.cmd_id);
		                      }
		                      break;
		                  case ELINK_CMD_ERR_SYNC:
		                      sendto(0, (uint8_t*)"ERR: SYNC WORD\r\n", 16, target_ip, target_port);
		                      printf("Sync error.\r\n");
		                      break;
		                  case ELINK_CMD_ERR_CHECKSUM:
		                      sendto(0, (uint8_t*)"ERR: CHECKSUM\r\n", 15, target_ip, target_port);
		                      printf("Checksum Error.\r\n");
		                      break;
		                  case ELINK_CMD_ERR_UNKNOWN_ID:
		                      sendto(0, (uint8_t*)"ERR: UNKNOWN ID\r\n", 17, target_ip, target_port);
		                      printf("Command %d doesn't exist.\r\n", incoming_cmd.cmd_id);
		                      break;
		                  case ELINK_CMD_ERR_PAYLOAD:
		                      sendto(0, (uint8_t*)"ERR: PAYLOAD\r\n", 14, target_ip, target_port);
		                      printf("Invalid payload size %u for command %u.\r\n",
		                             incoming_cmd.payload_size, incoming_cmd.cmd_id);
		                      break;
		              }
		          }
		          else {
		              printf("Wrong Size (%ld bytes, expected %d).\r\n", len, sizeof(ELinkCommand));
		          }
		      }
		  osDelay(10);

	  }
  /* USER CODE END StartRXELink */
}

/* USER CODE BEGIN Header_StartTXELINK */
/**
* @brief Function implementing the TXELINK thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTXELINK */
void StartTXELINK(void *argument)
{
  /* USER CODE BEGIN StartTXELINK */
	TelemetryPacket rx_packet;
	uint8_t target_ip[4] = GS_IP;
	uint16_t target_port = GS_PORT;

	osEventFlagsWait(InitFlags, FLAG_ETHERNET_OK,
	                 osFlagsWaitAny, osWaitForever);

	ELink_EnsureTxSocket();

  /* Infinite loop */
  for(;;)
  {
	  if (osMessageQueueGet(txElinkMailBoxHandle, &rx_packet,
	                        NULL, osWaitForever) == osOK) {
		  /* Recalculate immediately before transmission in case the packet is
		   * modified by future processing stages. */
		  Telemetry_Finalize(&rx_packet);
		  ELink_EnsureTxSocket();
		  int32_t sent_bytes = sendto(1, (uint8_t *)&rx_packet,
		                                sizeof(TelemetryPacket),
		                                target_ip, target_port);
		  if (sent_bytes < 0) {
			  uint8_t socket_state = getSn_SR(1);
			  printf("Error transmitting Telemetry via ELINK (%ld), socket 0x%02X.\r\n",
			         (long)sent_bytes, socket_state);

			  /* A destination timeout leaves UDP usable. Reopen only when the
			   * W5500 itself reports that socket 1 is no longer UDP. */
			  if (socket_state != SOCK_UDP) {
				  close(1);
				  ELink_EnsureTxSocket();
			  }
		  }
	  }
  }
  /* USER CODE END StartTXELINK */
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
