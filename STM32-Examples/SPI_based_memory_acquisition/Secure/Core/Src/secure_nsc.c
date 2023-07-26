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

void *pNonSecureToSecureTransferCompleteCallback = NULL; /* Pointer to callback in Non-secure */
void *pNonSecureToNonSecureTransferCompleteCallback = NULL; /* Pointer to callback in Non-secure */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Secure service to send secure memory buffer to secure SPI port. Also receives data in rx buffer
  * @retval SUCCESS or ERROR
  */
CMSE_NS_ENTRY ErrorStatus SECURE_SPI_Send_Data(){
	/*##-1- Start the Full Duplex Communication process ########################*/
	printf("Sending buffer to the server...\n\r");
	if (HAL_SPI_TransmitReceive_DMA(&hspi3, (uint8_t *)SEC_Mem_Buffer, (uint8_t *)aRxBuffer, BUFFER_SIZE) != HAL_OK)
	 {
		 /* Transfer error in transmission process */
		 printf("There was an error in starting up txrx.\n\r");
		 Error_Handler();
	 }

	 /*##-2- Wait for the end of the transfer ###################################*/
	 while (wTransferState == TRANSFER_WAIT)
	 {
	 }

	//check SpI transfer state
	 switch (wTransferState)
	 {
		 case TRANSFER_COMPLETE:
		   break;
		 default :
		   printf("There was an error in SPI transfer.\n\r");
		   Error_Handler();
		   break;
	 }
}


/**
  * @brief  Secure service to toggle SPI communication on or off. Must be called @ beginning and end of SPI transmission
  * @param state	SPI ON: 0 and SPI OFF: 1
  * @retval SUCCESS or ERROR
  */
CMSE_NS_ENTRY ErrorStatus SECURE_SPI_Toggle_Comm(int state){
	if(state > 0){
		//off state
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);

	}else{
		//on state
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
	}
}



/**
  * @brief  Secure service to get non-secure data by dedicated DMA channel
  *         under interrupt and with callback notification to non-secure
  * @param  nsc_mem_buffer		pointer to non-secure source buffer containing 256 words of memory (1024 bytes)
  * @param  Size        		requested size of data (256 words)
  * @param  func   				pointer to non-secure callback function on transfer end
  * @retval SUCCESS or ERROR
  */
CMSE_NS_ENTRY ErrorStatus SECURE_DMA_Fetch_NonSecure_Mem(uint32_t *nsc_mem_buffer,
                                                         uint32_t Size,
                                                         void *func)
{
  ErrorStatus  ret = ERROR;

  /* Check that the address range in non-secure */
   if (cmse_check_address_range(nsc_mem_buffer, Size * sizeof(uint32_t), CMSE_NONSECURE))
   {
	   //if so, start the transfer
	   /* Save callback function */
	    pNonSecureToSecureTransferCompleteCallback = func;
	    //start the DMA transfer
	    if (HAL_DMA_Start_IT(&hdma_memtomem_dma1_channel1,
	                             (uint32_t)nsc_mem_buffer,
	                             (uint32_t)&SEC_Mem_Buffer,
	                             BUFFER_SIZE) == HAL_OK)
		{
		  /* Transfer started */
		  ret = SUCCESS;
		}else{
			printf("transfer was not able to start.\n\r");
		}

   }

  return ret;
}



/**
  * @brief  Secure service to copy non-secure data into a buffer in non-secure; uses dedicated DMA channel
  * @param  src_buffer  		pointer to non-secure source buffer
  * @param dest_buffer			pointer to non-secure destination buffer
  * @param  size        		requested size of data (256 words)
  * @param  func   				pointer to non-secure callback function on transfer end
  * @retval SUCCESS or ERROR
  */
CMSE_NS_ENTRY ErrorStatus SECURE_DMA_NonSecure_Mem_Transfer(uint32_t *src_buffer, uint32_t *dest_buffer, uint32_t size, void* func){
	ErrorStatus ret = ERROR;

	 if (cmse_check_address_range(src_buffer, size * sizeof(uint32_t), CMSE_NONSECURE) && cmse_check_address_range(dest_buffer, size * sizeof(uint32_t), CMSE_NONSECURE)){
		 	 //if so, start the transfer
		   /* Save callback function */
			pNonSecureToNonSecureTransferCompleteCallback = func;
			//start the DMA transfer
			if (HAL_DMA_Start_IT(&hdma_memtomem_dma1_channel2,
									 (uint32_t)src_buffer,
									 (uint32_t)dest_buffer,
									 BUFFER_SIZE) == HAL_OK)
			{
			  /* Transfer started */
			  ret = SUCCESS;
			}else{
				printf("transfer was not able to start.\n\r");
			}
	 }else{
		 printf("Error: One or both of the buffers supplied are not in non-secure memory range.\n\r");
	 }

	 return ret;
}



//secure service for printing out buffers
CMSE_NS_ENTRY ErrorStatus SECURE_print_Buffer(uint32_t * buf, uint32_t size){
	//print the contents of the first transfer
	 printf("Non-secure memory buffer: \n\r");
	 printf("===========================================================================================================\n\r");
	 for(int i = 0; i<size; i++){
		printf("0x%08x\t", buf[i]);
		if((i+1)%4==0){
			printf("\n\r");
		}
	 }
	 printf("\n\r");
	 return SUCCESS;
}


//secure service for logging messages
CMSE_NS_ENTRY void SECURE_print_Log(char* string){
	printf(string);
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



//used in non-secure environment to check the secure memory buffer for successful transfer.
CMSE_NS_ENTRY ErrorStatus SECURE_DATA_Last_Buffer_Compare(uint32_t* addr)
{
	//used after non-secure to secure mem transfer
  ErrorStatus  ret = SUCCESS;
  //print the contents of the first transfer
	 printf("=====================================================================================================================\n\r");
	 for(int i = 0; i<BUFFER_SIZE; i++){
		 if((i+1)%16==1){
			 printf("%08p|\t", addr);
		 }
		printf("0x%02x\t", SEC_Mem_Buffer[i]);
		if((i+1)%16==0){
			printf("\n\r");
		}
		addr+=1;
	 }
	 printf("\n\r");


  return ret;
}



//callback functions
/**
  * @brief  DMA conversion complete callback
  * @note   This function is executed when the transfer complete interrupt
  *         is generated
  * @retval None
  */
void NonSecureToSecureTransferComplete(DMA_HandleTypeDef *hdma_memtomem_dma1_channel3)
{
  funcptr_NS callback_NS; // non-secure callback function pointer

  if(pNonSecureToSecureTransferCompleteCallback != (void *)NULL)
  {
   /* return function pointer with cleared LSB */
   callback_NS = (funcptr_NS)cmse_nsfptr_create(pNonSecureToSecureTransferCompleteCallback);

   callback_NS();
  }
  else
  {
    Error_Handler();  /* Something went wrong */
  }
}

/**
  * @brief  DMA conversion error callback
  * @note   This function is executed when the transfer error interrupt
  *         is generated during DMA transfer
  * @retval None
  */
void NonSecureToSecureTransferError(DMA_HandleTypeDef *hdma_memtomem_dma1_channel3)
{
  /* Error detected by secure application */
  Error_Handler();
}


/**
  * @brief  DMA conversion complete callback
  * @note   This function is executed when the transfer complete interrupt
  *         is generated
  * @retval None
  */
void NonSecureToNonSecureTransferComplete(DMA_HandleTypeDef *hdma_memtomem_dma1_channel2)
{
  funcptr_NS callback_NS; // non-secure callback function pointer

  if(pNonSecureToNonSecureTransferCompleteCallback != (void *)NULL)
  {
   /* return function pointer with cleared LSB */
   callback_NS = (funcptr_NS)cmse_nsfptr_create(pNonSecureToNonSecureTransferCompleteCallback);

   callback_NS();
  }
  else
  {
    Error_Handler();  /* Something went wrong */
  }
}

/**
  * @brief  DMA conversion error callback
  * @note   This function is executed when the transfer error interrupt
  *         is generated during DMA transfer
  * @retval None
  */
void NonSecureToNonSecureTransferError(DMA_HandleTypeDef *hdma_memtomem_dma1_channel2)
{
  /* Error detected by secure application */
  Error_Handler();
}

/**
  * @}
  */

/**
  * @}
  */
/* USER CODE END Non_Secure_CallLib */

