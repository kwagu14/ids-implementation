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
void SECURE_SPI_Send_Signal(uint8_t* signal, int size);
void SECURE_SPI_Send_Size(uint8_t* numModified);
void SECURE_SPI_Send_Data_Block(uint32_t addr);
void SECURE_SPI_Toggle_Comm(int state);
void SECURE_DMA_Fetch_NonSecure_Mem(uint32_t *nsc_mem_buffer, uint32_t Size);
void SECURE_SPI_Send_Block_List(char* blockList, int size);

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
CMSE_NS_ENTRY void SECURE_print_Num(char* string, int num){
	printf("%s %d\n\r", string, num);
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

CMSE_NS_ENTRY void SECURE_Send_Mem(){

	int modifiedBlockNums[256] = {0, 1, 10, 21, 100};
	int numModified = 5;
//	int numModified = 0;
	blockNum = 0;
	int max_blocks_modified = 8;
	int max_block_list_size = 3 * max_blocks_modified + max_blocks_modified;


	//loop through the blocks and compute hashes
//	uint32_t* current_address = (uint32_t*) NSEC_MEM_START;
//	while((uint32_t) current_address <= NSEC_MEM_END && (NSEC_MEM_END - (uint32_t)current_address) +1 >= BUFFER_SIZE){
//		  //get the current block
//		  if(HAL_DMA_Start_IT(&hdma_memtomem_dma1_channel1, (uint32_t)current_address, (uint32_t)&SEC_Mem_Buffer, BUFFER_SIZE/4) != HAL_OK){
//			  printf("could not start the secure to secure memory transfer.\n\r");
//		   }
//		  //wait for completion
//		  while ((NonsecureToSecureTransferCompleteDetected == 0) &&
//				 (NonsecureToSecureTransferErrorDetected == 0));
//
//		  //compute the hash
//		  if(HAL_HASH_MD5_Start(&hhash, (uint8_t*)SEC_Mem_Buffer, BUFFER_SIZE, SEC_Mem_Digest, 0xff) != HAL_OK){
//			  printf("There's an issue with the hash operation\n\r");
//		  }
//
//		  //compare to hash in the table
//		  if(memcmp(SEC_Mem_Digest, SEC_Mem_Hashes[blockNum], 16) != 0){
//			  printf("The hash has changed.\n\r");
//			  //the hash has changed
//			  modifiedBlockNums[numModified] = blockNum;
//			  numModified ++;
//		  }
//		  //increment variables
//		  blockNum ++;
//		  current_address += BUFFER_SIZE/4;
//	  }

	  //if 8 or fewer blocks have changed, send them in a blocking manner to esp32
//	if(numModified <= 8 && numModified > 0){
		printf("%d blocks have changed\n\r", numModified);

		//build list of modified block numbers
		char formattedBlockNums[max_block_list_size];
		int inc = 0;
		int pos = 0;
		inc = snprintf(&formattedBlockNums[pos], max_block_list_size, "%d", modifiedBlockNums[0]);
		pos += inc;
		//send the block list out through spi as a comma separated list
		for(int i = 1; i < numModified; i++){
			inc = snprintf(&formattedBlockNums[pos], max_block_list_size-pos+1, ",%d",modifiedBlockNums[i]);
			pos+= inc;
		}

		//toggle spi communication
		SECURE_SPI_Toggle_Comm(0);
		//send the start size signal through SPI
		SECURE_SPI_Send_Signal(START_SIZE_SIG, START_SIZE_SIG_SIZE);
		HAL_Delay(800);
		//send the number of modified blocks
		SECURE_SPI_Send_Size((uint8_t*) &numModified);
		HAL_Delay(800);
		//end the end size signal
		SECURE_SPI_Send_Signal(END_SIZE_SIG, END_SIZE_SIG_SIZE);
		HAL_Delay(800);
		SECURE_SPI_Toggle_Comm(1);
		HAL_Delay(1000);
		//send size of modified block list
		SECURE_SPI_Toggle_Comm(0);
		//send the start size signal through SPI
		SECURE_SPI_Send_Signal(START_BLOCK_LIST_SIZE_SIG, START_BLOCK_LIST_SIZE);
		HAL_Delay(800);
		//send the number of modified blocks
		SECURE_SPI_Send_Size((uint8_t*) &pos);
		HAL_Delay(800);
		//end the end size signal
		SECURE_SPI_Send_Signal(END_BLOCK_LIST_SIZE_SIG, END_BLOCK_LIST_SIZE);
		HAL_Delay(800);
		SECURE_SPI_Toggle_Comm(1);
		//send the modified block list
		SECURE_SPI_Toggle_Comm(0);
		SECURE_SPI_Send_Signal(START_BLOCK_NUMS_SIG, START_BLOCK_NUMS_SIZE);
		HAL_Delay(800);
		SECURE_SPI_Send_Block_List(formattedBlockNums, pos);
		HAL_Delay(800);
		SECURE_SPI_Send_Signal(END_BLOCK_NUMS_SIG, END_BLOCK_NUMS_SIZE);
		HAL_Delay(800);
		SECURE_SPI_Toggle_Comm(1);
		HAL_Delay(1000);
		SECURE_SPI_Toggle_Comm(0);
		//send the start signal
		SECURE_SPI_Send_Signal(START_TRANSMISSION_SIG, START_TRANS_SIZE);
		//now send each of the blocks
		for(int i = 0; i < numModified; i++){
			//calculate the address of the next block to send
			uint32_t addrOfNextBlock = NSEC_MEM_START + modifiedBlockNums[i]*BUFFER_SIZE;
			//send through SPI
			SECURE_SPI_Send_Data_Block(addrOfNextBlock);
		}
		//send the end signal
		SECURE_SPI_Send_Signal(END_TRANSMISSION_SIG, END_TRANS_SIZE);
		//toggle spi communication
		SECURE_SPI_Toggle_Comm(1);
//	}else if(numModified > 8){
//		printf("The number of blocks modified is greater than 8.\n\r");
//	}else if(numModified == 0){
//		printf("The number of blocks modified is equal to zero.\n\r");
//	}else{
//		printf("numBlocks is an invalid (negative) integer.\n\r");
//	}

}



//sends the start transmission signal out through the SPI Stream
//this allows the esp32 to know that the data it is receiving is the number of modifed memory blocks
void SECURE_SPI_Send_Signal(uint8_t* signal, int size){
	wTransferState = TRANSFER_WAIT;

	printf("Sending the %s signal...\n\r", (char*)signal);
	if (HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *)signal, size) != HAL_OK)
	 {
		 /* Transfer error in transmission process */
		 printf("There was an error in starting up tx.\n\r");
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
		   printf("The %s signal was successfully transmitted\n\r", (char*)signal);
		   break;
		 default :
		   Error_Handler();
		   break;
	 }
}



//sends the end size signal out through the SPI Stream
//this allows the esp32 to know that the data it is receiving is the number of modified memory blocks
//void SECURE_SPI_Send_End_Size_Signal(){
//	wTransferState = TRANSFER_WAIT;
//
//	printf("Sending the start size signal...\n\r");
//	if (HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *)END_SIZE_SIG, END_SIZE_SIG_SIZE) != HAL_OK)
//	 {
//		 /* Transfer error in transmission process */
//		 printf("There was an error in starting up tx.\n\r");
//		 Error_Handler();
//	 }
//
//	 /*##-2- Wait for the end of the transfer ###################################*/
//	 while (wTransferState == TRANSFER_WAIT)
//	 {
//	 }
//
//	//check SpI transfer state
//	 switch (wTransferState)
//	 {
//		 case TRANSFER_COMPLETE:
//		   printf("The end size signal was successfully transmitted\n\r");
//		   break;
//		 default :
//		   Error_Handler();
//		   break;
//	 }
//}



//sends the number of modifed memory blocks out through the SPI Stream
//this allows the esp32 to know how many memory blocks it needs to receive
void SECURE_SPI_Send_Size(uint8_t* numModified){
	wTransferState = TRANSFER_WAIT;

	printf("Sending the start size signal...\n\r");
	if (HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *) numModified, 1) != HAL_OK)
	 {
		 /* Transfer error in transmission process */
		 printf("There was an error in starting up tx.\n\r");
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
		   printf("The number of modified blocks was successfully transmitted\n\r");
		   break;
		 default :
		   Error_Handler();
		   break;
	 }
}



void SECURE_SPI_Send_Data_Block(uint32_t addr){

	//fetch the memory
	SECURE_DMA_Fetch_NonSecure_Mem((uint32_t*)addr, BUFFER_SIZE);

	wTransferState = TRANSFER_WAIT;
	/*** send the current memory block from the non-secure flash bank to SPI ***/
	if (HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *) SEC_Mem_Buffer, BUFFER_SIZE) != HAL_OK)
	 {
		 /* Transfer error in transmission process */
		 printf("There was an error in starting up tx.\n\r");
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
		   printf("The memory block was transferred successfully.");
		   break;
		 default :
		   printf("There was an error in transferring the memory dump.\n\r");
		   Error_Handler();
		   break;
	 }

}



/**
  * @brief  Secure function to get non-secure data by dedicated DMA channel
  *
  * @param  nsc_mem_buffer		pointer to non-secure source buffer containing 256 words of memory (1024 bytes)
  * @param  Size        		requested size of data (256 words)
  * @param  func   				pointer to non-secure callback function on transfer end
  * @retval SUCCESS or ERROR
  */
void SECURE_DMA_Fetch_NonSecure_Mem(uint32_t *nsc_mem_buffer, uint32_t Size)
{

  /* Check that the address range in non-secure */
   if (cmse_check_address_range(nsc_mem_buffer, Size * sizeof(uint32_t), CMSE_NONSECURE))
   {
	   printf("memory dump found to be in range.\n\r");
	    if (HAL_DMA_Start_IT(&hdma_memtomem_dma1_channel1,
	                             (uint32_t)nsc_mem_buffer,
	                             (uint32_t)&SEC_Mem_Buffer,
	                             Size) == HAL_OK)
		{
		  /* Transfer started */
	    	printf("Transfer has started\n\r");
		}else{
			printf("transfer was not able to start.\n\r");
		}
   }else{
	   printf("Address out of range...\n\r");
   }

}



void SECURE_SPI_Send_Block_List(char* blockList, int size){
	wTransferState = TRANSFER_WAIT;

	//transmit via SPI
	if (HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *) blockList, size) != HAL_OK)
	 {
		 /* Transfer error in transmission process */
		 printf("There was an error in starting up tx.\n\r");
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
		   printf("The data was transferred successfully.");
		   break;
		 default :
		   printf("There was an error in transferring the data.\n\r");
		   Error_Handler();
		   break;
	 }
}



/**
  * @brief  Secure service to toggle SPI communication on or off. Must be called @ beginning and end of SPI transmission
  * @param state	SPI ON: 0 and SPI OFF: 1
  * @retval SUCCESS or ERROR
  */
void SECURE_SPI_Toggle_Comm(int state){
	if(state > 0){
		//off state
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);

	}else{
		//on state
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
	}
}



//sends the start transmission signal out through the SPI Stream
//this allows the esp32 to know where memory dump data starts
//void SECURE_SPI_Send_Start_Signal(){
//	wTransferState = TRANSFER_WAIT;
//
//	printf("Sending the start transmission signal...\n\r");
//	if (HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *)START_TRANSMISSION_SIG, START_TRANS_SIZE) != HAL_OK)
//	 {
//		 /* Transfer error in transmission process */
//		 printf("There was an error in starting up tx.\n\r");
//		 Error_Handler();
//	 }
//
//	 /*##-2- Wait for the end of the transfer ###################################*/
//	 while (wTransferState == TRANSFER_WAIT)
//	 {
//	 }
//
//	//check SpI transfer state
//	 switch (wTransferState)
//	 {
//		 case TRANSFER_COMPLETE:
//		   break;
//		 default :
//		   Error_Handler();
//		   break;
//	 }
//}


//sends the end transmission signal once a memory dump as fully gone out through SPI.
//this is used by the esp32 to know when a memory dump has been fully received
//void SECURE_SPI_Send_End_Signal(){
//	wTransferState = TRANSFER_WAIT;
//
//	printf("Sending the end transmission signal...\n\r");
//	if (HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *)END_TRANSMISSION_SIG, END_TRANS_SIZE) != HAL_OK)
//	 {
//		 /* Transfer error in transmission process */
//		 printf("There was an error in starting up tx.\n\r");
//		 Error_Handler();
//	 }
//
//	 /*##-2- Wait for the end of the transfer ###################################*/
//	 while (wTransferState == TRANSFER_WAIT)
//	 {
//	 }
//
//	//check SpI transfer state
//	 switch (wTransferState)
//	 {
//		 case TRANSFER_COMPLETE:
//		   break;
//		 default :
//		   Error_Handler();
//		   break;
//	 }
//}



//CMSE_NS_ENTRY void SECURE_Send_Mem_Block(){
//	//if this is the zeroth block, toggle communication, send start signal, and initialize values
//	if(blockNum == 0){
//		SECURE_SPI_Toggle_Comm(0);
//		SECURE_SPI_Send_Start_Signal();
//		numBytesSent = 0;
//		current_address = (uint32_t*) NSEC_MEM_START;
//	}
//	//check that we're still within range & have enough bytes to transfer
//	if(blockNum < 256){
//		if((uint32_t) current_address <= NSEC_MEM_END && (NSEC_MEM_END - (uint32_t)current_address) +1 >= BUFFER_SIZE){
//			//get memory into secure flash bank
//			SECURE_DMA_Fetch_NonSecure_Mem((uint32_t *)current_address, BUFFER_SIZE/4);
//			SECURE_Print_Mem_Buffer();
//			//send data to esp32 through spi
//			SECURE_SPI_Send_Data();
//			//increment bytes sent
//			numBytesSent += BUFFER_SIZE;
//		}
//	}
//
//	//check if the block number is the last one
//	printf("blocknumber is %lu\n\r", blockNum);
//	if(blockNum == 256){
//		//send the end signal
//		SECURE_SPI_Send_End_Signal();
//	}
//
//	if(blockNum == 257){
//		//receive classification
//		SECURE_SPI_Receive_Classification();
//		//print the number of bytes sent
//		printf("Total number of bytes sent: %d\n\r", numBytesSent);
//		blockNum = 0;
//		HAL_IWDG_Refresh(&hiwdg);
//		return;
//	}
//
//	//increment the block number
//	blockNum++;
//	current_address += BUFFER_SIZE/4;
//	HAL_IWDG_Refresh(&hiwdg);
//}


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

