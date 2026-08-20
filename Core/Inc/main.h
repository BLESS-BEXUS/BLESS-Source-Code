/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

typedef enum {
	STATE_INIT,
	STATE_IDDLE,
	STATE_FLIGHT
} SystemState_t;

extern volatile SystemState_t current_state;

typedef struct {
    int32_t lat;
    int32_t lon;
    uint16_t alt;
    int8_t fix_sats;
} GPSData_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define B1_EXTI_IRQn EXTI15_10_IRQn
#define GNSS_UART_RX_Pin GPIO_PIN_0
#define GNSS_UART_RX_GPIO_Port GPIOC
#define GNSS_UART_TX_Pin GPIO_PIN_1
#define GNSS_UART_TX_GPIO_Port GPIOC
#define ITEMP_ALERT_Pin GPIO_PIN_2
#define ITEMP_ALERT_GPIO_Port GPIOC
#define ETEMP_ALERT_Pin GPIO_PIN_3
#define ETEMP_ALERT_GPIO_Port GPIOC
#define HL_PWR_A_Pin GPIO_PIN_0
#define HL_PWR_A_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define SCK_F1_Pin GPIO_PIN_5
#define SCK_F1_GPIO_Port GPIOA
#define MISO_F1_Pin GPIO_PIN_6
#define MISO_F1_GPIO_Port GPIOA
#define MOSI_F1_Pin GPIO_PIN_7
#define MOSI_F1_GPIO_Port GPIOA
#define HALOW_UART_TX_Pin GPIO_PIN_4
#define HALOW_UART_TX_GPIO_Port GPIOC
#define HALOW_UART_RX_Pin GPIO_PIN_5
#define HALOW_UART_RX_GPIO_Port GPIOC
#define SCK_ETH_Pin GPIO_PIN_13
#define SCK_ETH_GPIO_Port GPIOB
#define MISO_ETH_Pin GPIO_PIN_14
#define MISO_ETH_GPIO_Port GPIOB
#define MOSI_ETH_Pin GPIO_PIN_15
#define MOSI_ETH_GPIO_Port GPIOB
#define CS_F2_Pin GPIO_PIN_7
#define CS_F2_GPIO_Port GPIOC
#define GPIO_RED_LED_Pin GPIO_PIN_8
#define GPIO_RED_LED_GPIO_Port GPIOC
#define GPIO_YELLOW_LED_Pin GPIO_PIN_9
#define GPIO_YELLOW_LED_GPIO_Port GPIOC
#define GPIO_GREEN_LED_Pin GPIO_PIN_9
#define GPIO_GREEN_LED_GPIO_Port GPIOA
#define CS_ETH_Pin GPIO_PIN_8
#define CS_ETH_GPIO_Port GPIOA
#define RD_DATA_Pin GPIO_PIN_10
#define RD_DATA_GPIO_Port GPIOA
#define RD_DATA_EXTI_IRQn EXTI15_10_IRQn
#define USB_DMINUS_Pin GPIO_PIN_11
#define USB_DMINUS_GPIO_Port GPIOA
#define USB_DPLUS_Pin GPIO_PIN_12
#define USB_DPLUS_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SCK_ETEMP_Pin GPIO_PIN_10
#define SCK_ETEMP_GPIO_Port GPIOC
#define MISO_ETEMP_Pin GPIO_PIN_11
#define MISO_ETEMP_GPIO_Port GPIOC
#define MISO_ETEMPC12_Pin GPIO_PIN_12
#define MISO_ETEMPC12_GPIO_Port GPIOC
#define CS_ETEMP_Pin GPIO_PIN_2
#define CS_ETEMP_GPIO_Port GPIOD
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define INT_ETH_Pin GPIO_PIN_4
#define INT_ETH_GPIO_Port GPIOB
#define RST_ETH_Pin GPIO_PIN_5
#define RST_ETH_GPIO_Port GPIOB
#define CS_F1_Pin GPIO_PIN_6
#define CS_F1_GPIO_Port GPIOB
#define SCL_SENSORS_Pin GPIO_PIN_8
#define SCL_SENSORS_GPIO_Port GPIOB
#define SDA_SENSORS_Pin GPIO_PIN_9
#define SDA_SENSORS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

#define UDP_PORT 5000

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
