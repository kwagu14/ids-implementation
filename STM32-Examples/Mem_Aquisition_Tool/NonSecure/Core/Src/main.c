/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * Modified by Karley W. for STM32L562E-DK; added support for UART logging
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

//this is the data that we'll send to the secure environment
uint32_t aSRC_Const_Buffer[32] =
{
  0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10,
  0x11121314, 0x15161718, 0x191A1B1C, 0x1D1E1F20,
  0x21222324, 0x25262728, 0x292A2B2C, 0x2D2E2F30,
  0x31323334, 0x35363738, 0x393A3B3C, 0x3D3E3F40,
  0x41424344, 0x45464748, 0x494A4B4C, 0x4D4E4F50,
  0x51525354, 0x55565758, 0x595A5B5C, 0x5D5E5F60,
  0x61626364, 0x65666768, 0x696A6B6C, 0x6D6E6F70,
  0x71727374, 0x75767778, 0x797A7B7C, 0x7D7E7F80
};
uint32_t NSC_Mem_Buffer[BUFFER_SIZE];


//flags
static __IO uint32_t transferCompleteDetected; /* Set to 1 if transfer is correctly completed */
static __IO uint32_t transferErrorDetected; /* Set to 1 if an error transfer is detected */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void MX_GPIO_Init(void);
void SystemClock_Config(void);
//static void NonSecureTransferComplete(DMA_HandleTypeDef *hdma_memtomem_dma1_channel4);
//static void NonSecureTransferError(DMA_HandleTypeDef *hdma_memtomem_dma1_channel4);
//static void NonSecure_To_NonSecure_Mem_Transfer(uint32_t* src, uint32_t* dest, uint32_t size);

/* USER CODE BEGIN PFP */
static void NonSecureSecureTransferCompleteCallback(DMA_HandleTypeDef *hdma_memtomem_dma1_channelx);
static void NonSecureNonSecureTransferCompleteCallback(DMA_HandleTypeDef *hdma_memtomem_dma1_channelx);
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
  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */

  /* Step 3 */
    /* Provide non-secure data to secure */
    /* through secure DMA channels via Non-Secure Callable secure service */

  int remainder = 0;
  uint32_t* current_address = (uint32_t*) NSEC_MEM_START;
  //while we haven't reached the end of non-secure memory and we have at least 1024 bytes (256 words) to transfer
  while((uint32_t) current_address <= NSEC_MEM_END && (NSEC_MEM_END - (uint32_t)current_address) +1 >= BUFFER_SIZE*4){
	  	//move 1024 bytes into the memory buffer
  	  	  transferCompleteDetected = 0;
  	  	  if(SECURE_DMA_NonSecure_Mem_Transfer(current_address,
  	  			  	  	  	  	  	  	  	  	  (uint32_t*)NSC_Mem_Buffer,
												  (uint32_t) BUFFER_SIZE,
												  (void *)NonSecureNonSecureTransferCompleteCallback) == ERROR)
  	  	  {
  	  		SECURE_print_Log("There was an error with non-secure to secure transfer.\n\r");
  	  		Error_Handler();
  	  	  }

  	  	while (transferCompleteDetected == 0);

  	  	//SECURE_print_Buffer(NSC_Mem_Buffer, BUFFER_SIZE);

	    //perform a transfer to the secure environment
	    /* Reset transferCompleteDetected to 0, it will be set to 1 if a transfer is correctly completed */
	    transferCompleteDetected = 0;
	    if (SECURE_DMA_Fetch_NonSecure_Mem((uint32_t *)NSC_Mem_Buffer,
	                                       BUFFER_SIZE,
	                                       (void *)NonSecureSecureTransferCompleteCallback) == ERROR)
	    {
	    	SECURE_print_Log("There was an error with non-secure to secure transfer.\n\r");
	    	Error_Handler();
	    }

	    /* Wait for notification completion */
	    while (transferCompleteDetected == 0);
	    //print out to screen
	    SECURE_DATA_Last_Buffer_Compare((uint32_t*)current_address);
	    //increment the address variable by 1024 bytes
	    current_address += BUFFER_SIZE;
  }
  //we incremented one too many before checking the while condition, so undo the last increment
  current_address -= BUFFER_SIZE;

  //check if there's anything left over
  if((NSEC_MEM_END - (uint32_t) current_address) + 1 > 0){
	  //how many words left over?
	  remainder = ((NSEC_MEM_END - (uint32_t)current_address) + 1)/4;
	  //clear out the buffer
	  for(int i = 0; i < BUFFER_SIZE; i++){
		  NSC_Mem_Buffer[i] = 0;
	  }
	  //put in the remainder
	  if(SECURE_DMA_NonSecure_Mem_Transfer(current_address,
										  (uint32_t*)NSC_Mem_Buffer,
										  (uint32_t) remainder,
										  (void *)NonSecureNonSecureTransferCompleteCallback) == ERROR)
	  {
		SECURE_print_Log("There was an error with non-secure to secure transfer.\n\r");
		Error_Handler();
	  }

  while (transferCompleteDetected == 0);

  //perform one last non-secure to secure transfer
	transferCompleteDetected = 0;
	if (SECURE_DMA_Fetch_NonSecure_Mem((uint32_t *)NSC_Mem_Buffer,
									   BUFFER_SIZE,
									   (void *)NonSecureSecureTransferCompleteCallback) == ERROR)
	{
		SECURE_print_Log("There was an error with non-secure to secure transfer.\n\r");
		Error_Handler();
	}

	/* Wait for notification completion */
	while (transferCompleteDetected == 0);

  }

  /* USER CODE END 2 */

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE0) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 55;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}




/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}


//static void NonSecure_To_NonSecure_Mem_Transfer(uint32_t* src, uint32_t* dest, uint32_t size){
//	  HAL_DMA_RegisterCallback(&hdma_memtomem_dma1_channel4, HAL_DMA_XFER_CPLT_CB_ID, NonSecureTransferComplete);
//	  HAL_DMA_RegisterCallback(&hdma_memtomem_dma1_channel4, HAL_DMA_XFER_ERROR_CB_ID, NonSecureTransferError);
//	 /* Reset global var transferCompleteDetected to 0, it will be set to 1 if a transfer is correctly completed */
//	  transferCompleteDetected = 0;
//	  /* Reset global vartransferErrorDetected to 0, it will be set to 1 if a transfer error is detected */
//	  transferErrorDetected = 0;
//
//	  /* Configure the source, destination and buffer size DMA fields and Start DMA channel transfer */
//	  /* Enable DMA TC and TE interrupts */
//	  if (HAL_DMA_Start_IT(&hdma_memtomem_dma1_channel4,
//	                       (uint32_t)&src,
//	                       (uint32_t)&dest,
//	                       size) != HAL_OK)
//	  {
//	    /* Transfer Error */
//	    Error_Handler();
//	  }
//
//	  /* Wait for end of DMA transfer */
//	  while ((transferCompleteDetected == 0) &&
//	         (transferErrorDetected == 0)){SECURE_print_Log("Waiting for interrupt to be serviced. \n\r");}
//
//	  if (transferErrorDetected == 1)
//	  {
//		SECURE_print_Log("There was an error in non-secure to non-secure memory transfer. \n\r");
//	    Error_Handler();  /* Infinite loop */
//	  }
//
//}


/* USER CODE BEGIN 4 */


/**
  * @brief  DMA non-secure to secure transfer complete callback
  * @note   This function is executed when the transfer complete interrupt
  *         is generated
  * @retval None
  */
static void NonSecureSecureTransferCompleteCallback(DMA_HandleTypeDef *hdma_memtomem_dma1_channelx)
{
  transferCompleteDetected = 1;
}


/**
  * @brief  DMA non-secure to secure transfer complete callback
  * @note   This function is executed when the transfer complete interrupt
  *         is generated
  * @retval None
  */
static void NonSecureNonSecureTransferCompleteCallback(DMA_HandleTypeDef *hdma_memtomem_dma1_channelx)
{
  transferCompleteDetected = 1;
}


/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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

#ifdef  USE_FULL_ASSERT
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
