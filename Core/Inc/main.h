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
#define SCK_ETH_Pin GPIO_PIN_5
#define SCK_ETH_GPIO_Port GPIOA
#define MISO_ETH_Pin GPIO_PIN_6
#define MISO_ETH_GPIO_Port GPIOA
#define MOSI_ETH_Pin GPIO_PIN_7
#define MOSI_ETH_GPIO_Port GPIOA
#define USART_TX_GNSS_Pin GPIO_PIN_10
#define USART_TX_GNSS_GPIO_Port GPIOB
#define USART_RX_GNSS_Pin GPIO_PIN_11
#define USART_RX_GNSS_GPIO_Port GPIOB
#define CS_F2_Pin GPIO_PIN_7
#define CS_F2_GPIO_Port GPIOC
#define CS_F1_Pin GPIO_PIN_9
#define CS_F1_GPIO_Port GPIOA
#define RD_DATA_Pin GPIO_PIN_10
#define RD_DATA_GPIO_Port GPIOA
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
#define CS_ETH_Pin GPIO_PIN_6
#define CS_ETH_GPIO_Port GPIOB
#define SCL_SENSORS_Pin GPIO_PIN_8
#define SCL_SENSORS_GPIO_Port GPIOB
#define SDA_SENSORS_Pin GPIO_PIN_9
#define SDA_SENSORS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
