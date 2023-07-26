/**
  ******************************************************************************
  * @file    stm32l552e_eval_sd.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32l552e_eval_sd.c driver.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32L552E_EVAL_SD_H
#define STM32L552E_EVAL_SD_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l552e_eval_conf.h"
#include "stm32l552e_eval_errno.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32L552E-EV
  * @{
  */

/** @addtogroup STM32L552E-EV_SD
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup STM32L552E-EV_SD_Exported_Types STM32L552E-EV SD Exported Types
  * @{
  */

/**
  * @brief SD Card information structure
  */
#define BSP_SD_CardInfo HAL_SD_CardInfoTypeDef

#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
typedef struct
{
  pSD_CallbackTypeDef  pMspInitCb;
  pSD_CallbackTypeDef  pMspDeInitCb;
}BSP_SD_Cb_t;
#endif /* (USE_HAL_SD_REGISTER_CALLBACKS == 1) */
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup STM32L552E-EV_SD_Exported_Constants STM32L552E-EV SD Exported Constants
  * @{
  */
#define SD_INSTANCES_NBR              1U

#ifndef SD_WRITE_TIMEOUT
#define SD_WRITE_TIMEOUT              250U
#endif

#ifndef SD_READ_TIMEOUT
#define SD_READ_TIMEOUT               100U
#endif

/**
  * @brief  SD transfer state definition
  */
#define SD_TRANSFER_OK                0U
#define SD_TRANSFER_BUSY              1U

/**
  * @brief  SD detect pin definitions
  */
#define SD_DETECT_PIN                 GPIO_PIN_10
#define SD_DETECT_GPIO_PORT           GPIOG
#define __SD_DETECT_GPIO_CLK_ENABLE() __HAL_RCC_GPIOG_CLK_ENABLE()

#define SD_DETECT_IRQn                EXTI10_IRQn
#define SD_DETECT_IRQHandler          EXTI10_IRQHandler

#define SD_DETECT_EXTI_LINE           EXTI_LINE_10

#define SD_DATATIMEOUT                100000000UL

#define SD_PRESENT                    1U
#define SD_NOT_PRESENT                0U

/* SD IRQ handler */
#define SDMMCx_IRQHandler             SDMMC1_IRQHandler
#define SDMMCx_IRQn                   SDMMC1_IRQn

/**
  * @}
  */

/* Exported variables --------------------------------------------------------*/

/** @defgroup STM32L552E-EV_SD_Exported_Variables STM32L552E-EV SD Exported Variables
  * @{
  */
extern SD_HandleTypeDef hsd_sdmmc[SD_INSTANCES_NBR];

extern EXTI_HandleTypeDef hsd_exti[SD_INSTANCES_NBR];

extern uint32_t Sd_PinDetect[SD_INSTANCES_NBR];
extern GPIO_TypeDef* Sd_PortDetect[SD_INSTANCES_NBR];
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @defgroup STM32L552E-EV_SD_Exported_Functions STM32L552E-EV SD Exported Functions
  * @{
  */
int32_t BSP_SD_Init(uint32_t Instance);
int32_t BSP_SD_DeInit(uint32_t Instance);
#if (USE_HAL_SD_REGISTER_CALLBACKS == 1)
int32_t BSP_SD_RegisterDefaultMspCallbacks(uint32_t Instance);
int32_t BSP_SD_RegisterMspCallbacks(uint32_t Instance, BSP_SD_Cb_t *CallBacks);
#endif /* (USE_HAL_SD_REGISTER_CALLBACKS == 1) */
int32_t BSP_SD_DetectITConfig(uint32_t Instance);
void    BSP_SD_DETECT_IRQHandler(uint32_t Instance);
void    BSP_SD_IRQHandler(uint32_t Instance);
int32_t BSP_SD_ReadBlocks(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr);
int32_t BSP_SD_WriteBlocks(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr);
int32_t BSP_SD_ReadBlocks_DMA(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr);
int32_t BSP_SD_WriteBlocks_DMA(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr);
int32_t BSP_SD_ReadBlocks_IT(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr);
int32_t BSP_SD_WriteBlocks_IT(uint32_t Instance, uint32_t *pData, uint32_t BlockIdx, uint32_t BlocksNbr);
int32_t BSP_SD_Erase(uint32_t Instance, uint32_t BlockIdx, uint32_t BlocksNbr);
int32_t BSP_SD_GetCardState(uint32_t Instance);
int32_t BSP_SD_GetCardInfo(uint32_t Instance, BSP_SD_CardInfo *CardInfo);
int32_t BSP_SD_IsDetected(uint32_t Instance);

/* These functions can be modified by application code in case the current settings
   (eg. interrupt priority, callbacks implementation) need to be changed for specific application needs */
void    BSP_SD_AbortCallback(uint32_t Instance);
void    BSP_SD_WriteCpltCallback(uint32_t Instance);
void    BSP_SD_ReadCpltCallback(uint32_t Instance);
void    BSP_SD_DetectCallback(uint32_t Instance, uint32_t Status);

HAL_StatusTypeDef MX_SDMMC1_SD_Init(SD_HandleTypeDef *hsd);
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32L552E_EVAL_SD_H */
