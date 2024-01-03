/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined ( __ICCARM__ )
#  define CMSE_NS_CALL  __cmse_nonsecure_call
#  define CMSE_NS_ENTRY __cmse_nonsecure_entry
#else
#  define CMSE_NS_CALL  __attribute((cmse_nonsecure_call))
#  define CMSE_NS_ENTRY __attribute((cmse_nonsecure_entry))
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* Function pointer declaration in non-secure*/
#if defined ( __ICCARM__ )
typedef void (CMSE_NS_CALL *funcptr)(void);
#else
typedef void CMSE_NS_CALL (*funcptr)(void);
#endif

/* typedef for non-secure callback functions */
typedef funcptr funcptr_NS;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern uint8_t SEC_Mem_Digest[16];
extern HASH_HandleTypeDef hhash;
extern IWDG_HandleTypeDef hiwdg;
extern DMA_HandleTypeDef hdma_memtomem_dma1_channel1;
extern DMA_HandleTypeDef hdma_memtomem_dma1_channel4;
extern SPI_HandleTypeDef hspi3;
extern __IO uint32_t wTransferState;
extern uint8_t SEC_Mem_Hashes[256][16];
#define BUFFER_SIZE 1024U
extern uint8_t SEC_Mem_Buffer[BUFFER_SIZE];
extern uint8_t aRxBuffer[BUFFER_SIZE];
extern uint8_t START_CLASSIFICATION_SIG[];
extern uint8_t END_CLASSIFICATION_SIG[];
extern uint8_t START_SIZE_SIG[];
extern uint8_t END_SIZE_SIG[];
extern uint8_t END_TRANSMISSION_SIG[];
extern uint8_t START_TRANSMISSION_SIG[];
extern uint8_t START_BLOCK_NUMS_SIG[];
extern uint8_t END_BLOCK_NUMS_SIG[];
extern uint8_t START_BLOCK_LIST_SIZE_SIG[];
extern uint8_t END_BLOCK_LIST_SIZE_SIG[];
extern int END_CLASSIFICATION_SIZE;
extern int START_CLASSIFICATION_SIZE;
extern int START_SIZE_SIG_SIZE;
extern int END_SIZE_SIG_SIZE;
extern int START_TRANS_SIZE;
extern int END_TRANS_SIZE;
extern int MEM_DUMP_SIZE;
extern int START_BLOCK_NUMS_SIZE;
extern int END_BLOCK_NUMS_SIZE;
extern int START_BLOCK_LIST_SIZE;
extern int END_BLOCK_LIST_SIZE;
extern uint32_t blockNum;
extern int numBytesSent;
extern uint32_t* current_address;
extern uint8_t mem_dump_due;
extern uint8_t mem_dump_in_prog;
extern __IO uint32_t NonsecureToSecureTransferCompleteDetected;
extern __IO uint32_t NonsecureToSecureTransferErrorDetected;
extern uint32_t iter;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void NonSecureSecureTransferCompleteCallback(DMA_HandleTypeDef *hdma_memtomem_dma1_channelx);
void NonSecureToSecureTransferComplete(DMA_HandleTypeDef *hdma_memtomem_dma1_channel1);
void NonSecureToSecureTransferError(DMA_HandleTypeDef *hdma_memtomem_dma1_channel21);
int SearchForSig(uint8_t* signal, int sizeOfSig, uint8_t* data, int dataSize);
void SECURE_SPI_Receive_Classification();
//void SECURE_SPI_Send_Start_Signal();
//void SECURE_SPI_Send_End_Signal();
//void SECURE_SPI_Send_Data();
//void SECURE_SPI_Toggle_Comm(int state);
void SECURE_DMA_Fetch_NonSecure_Mem(uint32_t *nsc_mem_buffer, uint32_t Size);
void SECURE_Print_Mem_Buffer();


/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USART1_TX_Pin GPIO_PIN_9
#define USART1_TX_GPIO_Port GPIOA
#define USART1_RX_Pin GPIO_PIN_10
#define USART1_RX_GPIO_Port GPIOA
#define WHITE_LED_Pin GPIO_PIN_6
#define WHITE_LED_GPIO_Port GPIOC
#define IR_SENSOR_PIN_Pin GPIO_PIN_11
#define IR_SENSOR_PIN_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */
#define NSEC_MEM_START 0x08040000UL
#define NSEC_MEM_END 0x0807FFFFUL

#if defined(__ARMCC_VERSION) || defined(__ICCARM__)
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#elif defined(__GNUC__)
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#endif

enum
{
  TRANSFER_WAIT,
  TRANSFER_COMPLETE,
  TRANSFER_ERROR
};
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
