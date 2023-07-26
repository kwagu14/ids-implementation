/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    Secure_nsclib/secure_nsc.h
  * @author  MCD Application Team
  * @brief   Header for secure non-secure callable APIs list
  ******************************************************************************
    * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* USER CODE BEGIN Non_Secure_CallLib_h */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SECURE_NSC_H
#define SECURE_NSC_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  non-secure callback ID enumeration definition
  */
typedef enum
{
  SECURE_FAULT_CB_ID     = 0x00U, /*!< System secure fault callback ID */
  GTZC_ERROR_CB_ID       = 0x01U  /*!< GTZC secure error callback ID */
} SECURE_CallbackIDTypeDef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void SECURE_RegisterCallback(SECURE_CallbackIDTypeDef CallbackId, void *func);

void SECURE_print_Log(char* string);
ErrorStatus SECURE_DMA_Fetch_NonSecure_Mem(uint32_t *nsc_mem_buffer, uint32_t Size, void *pCallback);
ErrorStatus SECURE_DATA_Last_Buffer_Compare(uint32_t* addr);
ErrorStatus SECURE_print_Buffer(uint32_t * buf, uint32_t size);
ErrorStatus SECURE_DMA_NonSecure_Mem_Transfer(uint32_t *src_buffer, uint32_t *dest_buffer, uint32_t size, void* func);
ErrorStatus SECURE_SPI_Send_Data();
ErrorStatus SECURE_SPI_Toggle_Comm(int state);

#endif /* SECURE_NSC_H */
/* USER CODE END Non_Secure_CallLib_h */

