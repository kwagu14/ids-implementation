/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    Secure/Src/secure_nsc.c
  * @author  MCD Application Team
  * @brief   This file contains the non-secure callable APIs (secure world)
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

/* USER CODE BEGIN Non_Secure_CallLib */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "secure_nsc.h"
/** @addtogroup STM32L5xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Global variables ----------------------------------------------------------*/
void *pSecureFaultCallback = NULL;   /* Pointer to secure fault callback in Non-secure */
void *pSecureErrorCallback = NULL;   /* Pointer to secure error callback in Non-secure */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/


//secure service for logging messages
CMSE_NS_ENTRY void SECURE_print_Log(char* string){
	printf(string);
}


//secure service for logging messages containing integer data
CMSE_NS_ENTRY void SECURE_print_Num(int num){
	printf("%d\n\r", num);
}

////A memory forensics function
//CMSE_NS_ENTRY void SECURE_Mem_Forensics(){
//
//}

//CMSE_NS_ENTRY void SECURE_Send_Mem_Dump(){
//	  /*** Step 1: toggle SPI communication ****/
//	  SECURE_SPI_Toggle_Comm(0);
//	  //send the start transmission signal through SPI
//	  SECURE_SPI_Send_Start_Signal();
//	  /*** Step 2: copy a block of non-secure memory into a non-secure buffer ****/
//	  int numBytesSent=0;
//	  uint32_t* current_address = (uint32_t*) NSEC_MEM_START;
//	    //while we haven't reached the end of non-secure memory and we have at least 1024 bytes to transfer
//	    while((uint32_t) current_address <= NSEC_MEM_END && (NSEC_MEM_END - (uint32_t)current_address) +1 >= BUFFER_SIZE){
//
//
//		 /*** Step 3: copy the block of non-secure memory into the secure memory region ****/
//	printf("Sending memory dump...\n\r");
//			SECURE_DMA_Fetch_NonSecure_Mem((uint32_t*)current_address, BUFFER_SIZE/4);
//			//print out to screen
//			SECURE_Print_Mem_Buffer((uint32_t*)current_address);
//
////			/*** Step 4: transfer the block WiFi module using SPI ****/
//			SECURE_SPI_Send_Data();
//			//increment the address variable by 1024 bytes
//			current_address += BUFFER_SIZE/4;
//			numBytesSent+= 1024;
//
//			HAL_Delay(1000);
//
//	    }
//	    //send the end transmission signal to SPI
//	    SECURE_SPI_Send_End_Signal();
//	    HAL_Delay(2000);
//	    //try to receive the classification
//	    SECURE_SPI_Receive_Classification();
//		SECURE_SPI_Toggle_Comm(1);
//	    SECURE_print_Log("Total number of bytes sent:\n\r");
//	    SECURE_print_Num(numBytesSent);
//}


CMSE_NS_ENTRY void SECURE_Send_Mem_Block(){
	//if this is the zeroth block, toggle communication, send start signal, and initialize values
	if(blockNum == 0){
		SECURE_SPI_Toggle_Comm(0);
		SECURE_SPI_Send_Start_Signal();
		numBytesSent = 0;
		current_address = (uint32_t*) NSEC_MEM_START;
	}
	//check that we're still within range & have enough bytes to transfer
	if(blockNum < 256){
		if((uint32_t) current_address <= NSEC_MEM_END && (NSEC_MEM_END - (uint32_t)current_address) +1 >= BUFFER_SIZE){
			//get memory into secure flash bank
			SECURE_DMA_Fetch_NonSecure_Mem((uint32_t *)current_address, BUFFER_SIZE/4);
			SECURE_Print_Mem_Buffer();
			//send data to esp32 through spi
			SECURE_SPI_Send_Data();
			//increment bytes sent
			numBytesSent += BUFFER_SIZE;
		}
	}

	//check if the block number is the last one
	printf("blocknumber is %d\n\r", blockNum);
	if(blockNum == 256){
		//send the end signal
		SECURE_SPI_Send_End_Signal();
	}

	if(blockNum == 257){
		//receive classification
		SECURE_SPI_Receive_Classification();
		//print the number of bytes sent
		printf("Total number of bytes sent: %d\n\r", numBytesSent);
		blockNum = 0;
		HAL_IWDG_Refresh(&hiwdg);
		return;
	}

	//increment the block number
	blockNum++;
	current_address += BUFFER_SIZE/4;
	HAL_IWDG_Refresh(&hiwdg);
}


/**
  * @brief  Secure registration of non-secure callback.
  * @param  CallbackId  callback identifier
  * @param  func        pointer to non-secure function
  * @retval None
  */
CMSE_NS_ENTRY void SECURE_RegisterCallback(SECURE_CallbackIDTypeDef CallbackId, void *func)
{
  if(func != NULL)
  {
    switch(CallbackId)
    {
      case SECURE_FAULT_CB_ID:           /* SecureFault Interrupt occurred */
        pSecureFaultCallback = func;
        break;
      case GTZC_ERROR_CB_ID:             /* GTZC Interrupt occurred */
        pSecureErrorCallback = func;
        break;
      default:
        /* unknown */
        break;
    }
  }
}

/**
  * @}
  */

/**
  * @}
  */
/* USER CODE END Non_Secure_CallLib */

