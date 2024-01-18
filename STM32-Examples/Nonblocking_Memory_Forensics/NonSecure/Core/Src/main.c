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
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
static __IO uint32_t transferCompleteDetected; /* Set to 1 if transfer is correctly completed */
static __IO uint32_t transferErrorDetected; /* Set to 1 if an error transfer is detected */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void MX_DMA_Init(void);
static void MX_GPIO_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
//void SystemClock_Config(void);
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
	uint8_t nsec_buf[261] = {
	                            0x54,0x68,0x65,0x20,0x68,0x61,0x73,0x68,0x20,0x70,
	                            0x72,0x6f,0x63,0x65,0x73,0x73,0x6f,0x72,0x20,0x69,
	                            0x73,0x20,0x61,0x20,0x66,0x75,0x6c,0x6c,0x79,0x20,
	                            0x63,0x6f,0x6d,0x70,0x6c,0x69,0x61,0x6e,0x74,0x20,
	                            0x69,0x6d,0x70,0x6c,0x65,0x6d,0x65,0x6e,0x74,0x61,
	                            0x74,0x69,0x6f,0x6e,0x20,0x6f,0x66,0x20,0x74,0x68,
	                            0x65,0x20,0x73,0x65,0x63,0x75,0x72,0x65,0x20,0x68,
	                            0x61,0x73,0x68,0x20,0x61,0x6c,0x67,0x6f,0x72,0x69,
	                            0x74,0x68,0x6d,0x20,0x28,0x53,0x48,0x41,0x2d,0x31,
	                            0x29,0x2c,0x20,0x74,0x68,0x65,0x20,0x4d,0x44,0x35,
	                            0x20,0x28,0x6d,0x65,0x73,0x73,0x61,0x67,0x65,0x2d,
	                            0x64,0x69,0x67,0x65,0x73,0x74,0x20,0x61,0x6c,0x67,
	                            0x6f,0x72,0x69,0x74,0x68,0x6d,0x20,0x35,0x29,0x20,
	                            0x68,0x61,0x73,0x68,0x20,0x61,0x6c,0x67,0x6f,0x72,
	                            0x69,0x74,0x68,0x6d,0x20,0x61,0x6e,0x64,0x20,0x74,
	                            0x68,0x65,0x20,0x48,0x4d,0x41,0x43,0x20,0x28,0x6b,
	                            0x65,0x79,0x65,0x64,0x2d,0x68,0x61,0x73,0x68,0x20,
	                            0x6d,0x65,0x73,0x73,0x61,0x67,0x65,0x20,0x61,0x75,
	                            0x74,0x68,0x65,0x6e,0x74,0x69,0x63,0x61,0x74,0x69,
	                            0x6f,0x6e,0x20,0x63,0x6f,0x64,0x65,0x29,0x20,0x61,
	                            0x6c,0x67,0x6f,0x72,0x69,0x74,0x68,0x6d,0x20,0x73,
	                            0x75,0x69,0x74,0x61,0x62,0x6c,0x65,0x20,0x66,0x6f,
	                            0x72,0x20,0x61,0x20,0x76,0x61,0x72,0x69,0x65,0x74,
	                            0x79,0x20,0x6f,0x66,0x20,0x61,0x70,0x70,0x6c,0x69,
	                            0x63,0x61,0x74,0x69,0x6f,0x6e,0x73,0x2e,0x2a,0x2a,
	                            0x2a,0x20,0x53,0x54,0x4d,0x33,0x32,0x20,0x2a,0x2a,
	                            0x2a};
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_DMA_Init();
  MX_GPIO_Init();
  MX_TIM4_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  //check memory one time
  SECURE_Send_Modified_Mem();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1){
//	  SECURE_print_Log("Hello.\n\r");
//	  current_ir_sensor_val = HAL_GPIO_ReadPin(IR_SENSOR_PIN_GPIO_Port, IR_SENSOR_PIN_Pin);
//	  //has the state changed?
//	  if(current_ir_sensor_val != previous_ir_sensor_val){
//		  SECURE_print_Log("State changed.\n\r");
//		  if(current_ir_sensor_val == GPIO_PIN_RESET){
//			  SECURE_print_Log("ENTITY APPROACHING\n\r");
//			  //let the server know; send this to esp32
//			  HAL_UART_Transmit(&huart3, (uint8_t*) ENTITY_APPROACHING, (uint16_t) MAX_NIC_MESSAGE_LEN, 0xFFFF);
//		  }else{
//			  SECURE_print_Log("ENTITY DEPARTED\n\r");
//			  //let the server know
//			  HAL_UART_Transmit(&huart3, (uint8_t*) ENTITY_DEPARTING, (uint16_t) MAX_NIC_MESSAGE_LEN, 0xFFFF);
//		  }
//		  HAL_Delay(1);
//	  }
//	  previous_ir_sensor_val = current_ir_sensor_val;
//	  if(current_ir_sensor_val == 0){
//		  HAL_GPIO_WritePin(WHITE_LED_GPIO_Port, WHITE_LED_Pin, GPIO_PIN_SET);
//	  }else{
//		  HAL_GPIO_WritePin(WHITE_LED_GPIO_Port, WHITE_LED_Pin, GPIO_PIN_RESET);
//	  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 10999;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 10000;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_ODD;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(WHITE_LED_GPIO_Port, WHITE_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : WHITE_LED_Pin */
  GPIO_InitStruct.Pin = WHITE_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(WHITE_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : IR_SENSOR_PIN_Pin */
  GPIO_InitStruct.Pin = IR_SENSOR_PIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(IR_SENSOR_PIN_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
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

//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
//      if (htim->Instance == TIM4) {
//              SECURE_Send_Mem_Block();
//      }
//}
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
