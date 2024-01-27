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
void SECURE_SPI_Send_Data_Buf(uint8_t* data, int size);
void SECURE_SPI_Send_Data_Block(uint32_t* addr);
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


//sends a full memory dump
CMSE_NS_ENTRY void SECURE_Send_Mem(){

	int blocksToSend = 256;
	uint32_t* current_addr;
	uint8_t blocks_to_send_bytes[2];
	blocks_to_send_bytes[0] = blocksToSend >> 8;
	blocks_to_send_bytes[1] = blocksToSend & 255;

	/********************************************* SEND NUM BLOCKS *******************************************************************************/

	//toggle spi communication; CS --> LOW (needed for library used on ESP32)
	SECURE_SPI_Toggle_Comm(0);
	//send the start size signal through SPI
	SECURE_SPI_Send_Signal(START_SIZE_SIG, START_SIZE_SIG_SIZE);
	HAL_Delay(800);
	//send the number of modified blocks encoded as an unsigned byte. Range of values are 0-255 but we need 1-256, so needs to be adjusted at ESP32 side.
	SECURE_SPI_Send_Data_Buf((uint8_t*) blocks_to_send_bytes, 2);
	HAL_Delay(800);
	//end the end size signal
	SECURE_SPI_Send_Signal(END_SIZE_SIG, END_SIZE_SIG_SIZE);
	HAL_Delay(800);
	SECURE_SPI_Toggle_Comm(1);

	/******************************************** SEND MEMORY BLOCKS THROUGH SPI STREAM ***********************************************************/
	for(int i = 0; i < 32; i++){
		//send an 8KB chunk
		SECURE_SPI_Toggle_Comm(0);
		SECURE_SPI_Send_Signal(START_TRANSMISSION_SIG, START_TRANS_SIZE);
		//start at new 8KB boundary
		current_addr = (uint32_t*)(NSEC_MEM_START + i*8192);
		for(int i = 0; i < 8; i++){
			//send through SPI
			printf("------------------------------------------------------------------------------------\n\r");
			printf("sending data at address %p\n\r", current_addr);
			SECURE_Print_Mem_Buffer(SEC_Mem_Buffer, 1024);
			printf("------------------------------------------------------------------------------------\n\r");
			SECURE_SPI_Send_Data_Block(current_addr);
			//calculate the address of the next block to send
			current_addr += BUFFER_SIZE/4;
		}
		//send the end signal
		SECURE_SPI_Send_Signal(END_TRANSMISSION_SIG, END_TRANS_SIZE);
		//toggle spi communication
		SECURE_SPI_Toggle_Comm(1);
		//decrement num modified blocks
		HAL_Delay(10000);
	}

}



//sends only the modified memory
CMSE_NS_ENTRY void SECURE_Send_Modified_Mem(){

	int modifiedBlockNums[256];
	uint16_t numModified = 0;

	//reset watchdog
	//HAL_IWDG_Refresh(&hiwdg);

	//used for testing
//	uint16_t numModified = 7;
//	int modifiedBlockNums[256] = {0, 1, 9, 15, 20, 21, 44};

	blockNum = 0;
	int max_blocks_modified = 256;
	int max_block_list_size = 3 * max_blocks_modified + max_blocks_modified;
	uint8_t block_list_size_bytes[2];
	uint8_t num_modified_bytes[2];

	/******************************************* ACQUIRE INFO ABOUT CHANGED BLOCKS *********************************************************/

	//loop through the blocks and compute hashes
	uint32_t* current_address = (uint32_t*) NSEC_MEM_START;
	while((uint32_t) current_address <= NSEC_MEM_END && (NSEC_MEM_END - (uint32_t)current_address) +1 >= BUFFER_SIZE){
		  //get the current block
		  if(HAL_DMA_Start_IT(&hdma_memtomem_dma1_channel1, (uint32_t)current_address, (uint32_t)&SEC_Mem_Buffer, BUFFER_SIZE/4) != HAL_OK){
			  printf("could not start the secure to secure memory transfer.\n\r");
		   }
		  //wait for completion
		  while ((NonsecureToSecureTransferCompleteDetected == 0) &&
				 (NonsecureToSecureTransferErrorDetected == 0));

		  //compute the hash
		  if(HAL_HASH_MD5_Start(&hhash, (uint8_t*)SEC_Mem_Buffer, BUFFER_SIZE, SEC_Mem_Digest, 0xff) != HAL_OK){
			  printf("There's an issue with the hash operation\n\r");
		  }

		  //compare to hash in the table
		  if(memcmp(SEC_Mem_Digest, SEC_Mem_Hashes[blockNum], 16) != 0){
			  printf("The hash has changed for block %d.\n\r", blockNum);
			  //the hash has changed
			  modifiedBlockNums[numModified] = blockNum;
			  numModified ++;
		  }
		  //increment variables
		  blockNum ++;
		  current_address += BUFFER_SIZE/4;
	  }

	/******************************************* DATA MANIPULATION *********************************************************/
	printf("%d blocks have changed\n\r", numModified);
	//build list of modified block numbers separated by commas. Encoded as a string
	char formattedBlockNums[max_block_list_size];
	int inc = 0;
	uint16_t pos = 0;
	inc = snprintf(&formattedBlockNums[pos], max_block_list_size, "%d", modifiedBlockNums[0]);
	pos += inc;
	//send the block list out through spi as a comma separated list
	for(int i = 1; i < numModified; i++){
		inc = snprintf(&formattedBlockNums[pos], max_block_list_size-pos+1, ",%d",modifiedBlockNums[i]);
		pos+= inc;
	}
	//encode block list size as a stream of bytes so it can be sent via SPI
	block_list_size_bytes[0] = pos >> 8; //most significant byte
	block_list_size_bytes[1] = pos & 255; //least significant byte
	//do the same for the count of modified blocks
	num_modified_bytes[0] = numModified >> 8;
	num_modified_bytes[1] = numModified & 255;

	/******************************************* BUILDING THE SPI DATA STREAM (METADATA) *********************************************************/

	//toggle spi communication; CS --> LOW (needed for library used on ESP32)
	SECURE_SPI_Toggle_Comm(0);
	//send the start size signal through SPI
	SECURE_SPI_Send_Signal(START_SIZE_SIG, START_SIZE_SIG_SIZE);
	HAL_Delay(500);
	//send the number of modified blocks encoded as an unsigned byte. Range of values are 0-255 but we need 1-256, so needs to be adjusted at ESP32 side.
	SECURE_SPI_Send_Data_Buf((uint8_t*) num_modified_bytes, 2);
	HAL_Delay(500);
	//end the end size signal
	SECURE_SPI_Send_Signal(END_SIZE_SIG, END_SIZE_SIG_SIZE);
	HAL_Delay(500);
	SECURE_SPI_Toggle_Comm(1);
	HAL_Delay(500);
	//send size of modified block list. This can potentially be a large number so we need to send it encoded as an unsigned short (two unsigned bytes)
	SECURE_SPI_Toggle_Comm(0);
	//send the start size signal through SPI
	SECURE_SPI_Send_Signal(START_BLOCK_LIST_SIZE_SIG, START_BLOCK_LIST_SIZE);
	HAL_Delay(500);
	//send the number of modified blocks
	SECURE_SPI_Send_Data_Buf((uint8_t*) block_list_size_bytes, 2);
	HAL_Delay(500);
	//end the end size signal
	SECURE_SPI_Send_Signal(END_BLOCK_LIST_SIZE_SIG, END_BLOCK_LIST_SIZE);
	HAL_Delay(500);
	SECURE_SPI_Toggle_Comm(1);
	//send the modified block list
	SECURE_SPI_Toggle_Comm(0);
	SECURE_SPI_Send_Signal(START_BLOCK_NUMS_SIG, START_BLOCK_NUMS_SIZE);
	HAL_Delay(500);
	SECURE_SPI_Send_Block_List(formattedBlockNums, pos);
	HAL_Delay(500);
	SECURE_SPI_Send_Signal(END_BLOCK_NUMS_SIG, END_BLOCK_NUMS_SIZE);
	HAL_Delay(500);
	SECURE_SPI_Toggle_Comm(1);
	HAL_Delay(500);

	/******************************************** SEND MEMORY BLOCKS THROUGH SPI STREAM ***********************************************************/
	while(numModified > 0){
		//send an 8KB chunk
		SECURE_SPI_Toggle_Comm(0);
		SECURE_SPI_Send_Signal(START_TRANSMISSION_SIG, START_TRANS_SIZE);
		int blockInd = 0;
		for(int i = 0; i < (numModified >= 8 ? 8 : numModified); i++){
			//calculate the address of the next block to send
			uint32_t* addrOfNextBlock = (uint32_t*) NSEC_MEM_START + modifiedBlockNums[blockInd]*BUFFER_SIZE/4;
			printf("------------------------------------------------------------------------------------\n\r");
			printf("sending data at address %p\n\r", addrOfNextBlock);
			SECURE_Print_Mem_Buffer(SEC_Mem_Buffer, 1024);
			printf("------------------------------------------------------------------------------------\n\r");
			//send through SPI
			SECURE_SPI_Send_Data_Block(addrOfNextBlock);
			blockInd++;
		}
		//send the end signal
		SECURE_SPI_Send_Signal(END_TRANSMISSION_SIG, END_TRANS_SIZE);
		//decrement num modified blocks; be careful not to underflow
		numModified -= (numModified >= 8 ? 8 : numModified);
		//if all blocks sent, immediately exit. If not, need to delay before sending next one.
		if(numModified <= 0){
			break;
		}else{
			SECURE_SPI_Toggle_Comm(1);
			HAL_Delay(10000);
		}
	}
	int res = SECURE_SPI_Receive_Classification();
	while(!res){
		SECURE_SPI_Receive_Classification();
		HAL_Delay(100);
	}


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


//sends the number of modifed memory blocks out through the SPI Stream
//this allows the esp32 to know how many memory blocks it needs to receive
void SECURE_SPI_Send_Data_Buf(uint8_t* data, int size){
	wTransferState = TRANSFER_WAIT;

	printf("Sending the start size signal...\n\r");
	if (HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *) data, size) != HAL_OK)
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



void SECURE_SPI_Send_Data_Block(uint32_t* addr){

	//fetch the memory
	SECURE_DMA_Fetch_NonSecure_Mem((uint32_t*)addr, BUFFER_SIZE/4);

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
		   printf("The memory block was transferred successfully.\n\r");
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
	                             (uint32_t)SEC_Mem_Buffer,
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



//this function checks for & receives a classification from a stream of SPI data coming in through MISO
//It must be called in memory forensics after sending a full memory dump to the server
int SECURE_SPI_Receive_Classification(){

	int containsStartClassification;
	int containsEndClassification;
	int retval = 0;
	//perform a new rx operation
	wTransferState = TRANSFER_WAIT;
	if (HAL_SPI_Receive_DMA(&hspi3, (uint8_t *)aRxBuffer, BUFFER_SIZE) != HAL_OK)
	 {
		 /* Transfer error in transmission process */
		 printf("There was an error in starting up RX.\n\r");
		 Error_Handler();
	 }

	 /*##-2- Wait for the end of the transfer ###################################*/
	 while (wTransferState == TRANSFER_WAIT)
	 {
	 }

	//check SPI transfer state
	if(wTransferState == TRANSFER_COMPLETE){

		//print the buffer received.
//		printf("Buffer received from SPI: \n\r");
//		for(int i = 0; i < BUFFER_SIZE; i++){
//			printf("%d\t", aRxBuffer[i]);
//		}
//		printf("\n\r");

		//if the transfer completed, look for the start and end signals in the SPI rx buffer
		containsStartClassification = SearchForSig(START_CLASSIFICATION_SIG, START_CLASSIFICATION_SIZE, aRxBuffer, BUFFER_SIZE);
		containsEndClassification = SearchForSig(END_CLASSIFICATION_SIG, END_CLASSIFICATION_SIZE, aRxBuffer, BUFFER_SIZE);
		if(containsStartClassification >= 0 && containsEndClassification >= 0){
			//The start and end signals were found; make sure there's only one element between them
			if(containsStartClassification == containsEndClassification - START_CLASSIFICATION_SIZE - 1){
				retval = 1;
				//try to extract the classification
				printf("Classification found. Extracting...\n\r");
				int classificationInd = containsEndClassification - 1;
				uint8_t classification = aRxBuffer[classificationInd];
				printf("Classification: %c\n\r", classification);
				if(classification == '1'){
					printf("Memory was found to be benign.\n\r");
				}else if(classification == '0'){
					printf("Memory was found to be malicious.\n\r");
				}else{
					printf("Error in receiving classification from server. Bad value: %d\n\r", classification);
				}
			}else{
				printf("Error: the SPI classification stream is incorrectly formatted.\n\r");
				printf("Start classification: %d, End classification: %d\n\r", containsStartClassification, containsEndClassification);
			}
		}
	}else{
		//if transfer didn't complete
		Error_Handler();
	}
	return retval;

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

