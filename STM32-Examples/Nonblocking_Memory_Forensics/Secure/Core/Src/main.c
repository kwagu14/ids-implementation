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

/* Non-secure Vector table to jump to (internal Flash Bank2 here)             */
/* Caution: address must correspond to non-secure internal Flash where is     */
/*          mapped in the non-secure vector table                             */
#define VTOR_TABLE_NS_START_ADDR  0x08040000UL

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

HASH_HandleTypeDef hhash;

IWDG_HandleTypeDef hiwdg;

SPI_HandleTypeDef hspi3;
DMA_HandleTypeDef hdma_spi3_rx;
DMA_HandleTypeDef hdma_spi3_tx;

UART_HandleTypeDef huart1;

DMA_HandleTypeDef hdma_memtomem_dma1_channel1;

/************* NOT USED *******************/
DMA_HandleTypeDef hdma_memtomem_dma1_channel4;

/* USER CODE BEGIN PV */

//initial SPI transfer state
__IO uint32_t wTransferState = TRANSFER_WAIT;
//DMA transfer state

__IO uint32_t NonsecureToSecureTransferCompleteDetected;
__IO uint32_t NonsecureToSecureTransferErrorDetected;

/************* NOT USED *******************/
static __IO uint32_t SecureToSecureTransferErrorDetected;    /* Set to 1 if an error transfer is detected */
static __IO uint32_t SecureToSecureTransferCompleteDetected; /* Set to 1 if transfer is correctly completed */

//holds non-secure memory blocks before they get sent through SPI
uint8_t SEC_Mem_Buffer[BUFFER_SIZE];
uint8_t SEC_Mem_Digest[16];
//stores the last known hash of memory blocks
uint8_t SEC_Mem_Hashes[256][16];
//receives classification from server
uint8_t aRxBuffer[BUFFER_SIZE];
//these signals get sent to this board in the SPI Stream to delimit the classification
uint8_t START_CLASSIFICATION_SIG[] = "<START_CLASSIFICATION>";
uint8_t END_CLASSIFICATION_SIG[] = "<END_CLASSIFICATION>";
uint8_t END_TRANSMISSION_SIG[] = "<END_TRANSMISSION>";
uint8_t START_TRANSMISSION_SIG[] = "<START_TRANSMISSION>";
uint8_t START_SIZE_SIG[] = "<START_SIZE>";
uint8_t END_SIZE_SIG[] = "<END_SIZE>";
uint8_t START_BLOCK_NUMS_SIG[] = "<START_BLOCK_NUMS>";
uint8_t END_BLOCK_NUMS_SIG[] = "<END_BLOCK_NUMS>";
uint8_t START_BLOCK_LIST_SIZE_SIG[] = "<START_BLOCK_LIST_SIZE>";
uint8_t END_BLOCK_LIST_SIZE_SIG[] = "<END_BLOCK_LIST_SIZE>";
//the size of the above signals
int END_CLASSIFICATION_SIZE = 20;
int START_CLASSIFICATION_SIZE = 22;
int START_SIZE_SIG_SIZE = 12;
int END_SIZE_SIG_SIZE = 10;
int START_TRANS_SIZE = 20;
int END_TRANS_SIZE = 18;
int START_BLOCK_NUMS_SIZE = 18;
int END_BLOCK_NUMS_SIZE = 16;
int START_BLOCK_LIST_SIZE = 23;
int END_BLOCK_LIST_SIZE = 21;
//the total size of the memory dump (256KB)
int MEM_DUMP_SIZE = 262144;
//The block number to be sent next to the server
uint32_t blockNum = 0;
//number of bytes sent to server so far (0 if transfer hasn't started yet)
int numBytesSent = 0;

//current non-secure memory address for transfers
uint32_t* current_address = (uint32_t*) NSEC_MEM_START;
//this tells memory forensics if a memory transfer is currently happening
uint8_t mem_dump_in_prog = 0;
//tells memory forensics if a memory dump is due (if transfer not already happening)
uint8_t mem_dump_due = 0;
uint32_t iter = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void NonSecure_Init(void);
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ICACHE_Init(void);
static void MX_GTZC_S_Init(void);
static void MX_SPI3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_IWDG_Init(void);
static void MX_HASH_Init(void);
/* USER CODE BEGIN PFP */
static void NonsecureToSecureTransferComplete(DMA_HandleTypeDef *hdma_memtomem_dma1_channel1);
static void NonsecureToSecureTransferError(DMA_HandleTypeDef *hdma_memtomem_dma1_channel1);

/************* NOT USED *******************/
static void SecureToSecureTransferComplete(DMA_HandleTypeDef *hdma_memtomem_dma1_channel4);
static void SecureToSecureTransferError(DMA_HandleTypeDef *hdma_memtomem_dma1_channel4);

void SECURE_Print_Mem_Buffer(uint8_t* buf, int size);
void SECURE_DMA_Fetch_NonSecure_Mem(uint32_t *nsc_mem_buffer, uint32_t Size);

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
  /* SAU/IDAU, FPU and interrupts secure/non-secure allocation setup done */
  /* in SystemInit() based on partition_stm32l562xx.h file's definitions. */
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();
  /* GTZC initialisation */
  MX_GTZC_S_Init();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ICACHE_Init();
  MX_SPI3_Init();
  MX_USART1_UART_Init();
//MX_IWDG_Init();
  MX_HASH_Init();

  /* USER CODE BEGIN 2 */
  HAL_DMA_RegisterCallback(&hdma_memtomem_dma1_channel1, HAL_DMA_XFER_CPLT_CB_ID, NonsecureToSecureTransferComplete);
  HAL_DMA_RegisterCallback(&hdma_memtomem_dma1_channel1, HAL_DMA_XFER_ERROR_CB_ID, NonsecureToSecureTransferError);

  /************* NOT USED *******************/
  /* DMA1 Channel4: Select Callbacks functions called after Transfer complete and Transfer error */ /*NOT USED***********/
  HAL_DMA_RegisterCallback(&hdma_memtomem_dma1_channel4, HAL_DMA_XFER_CPLT_CB_ID, SecureToSecureTransferComplete);
  HAL_DMA_RegisterCallback(&hdma_memtomem_dma1_channel4, HAL_DMA_XFER_ERROR_CB_ID, SecureToSecureTransferError);


  /******************************* BUILD THE INITIAL MEMORY HASH TABLE *********************************************/
  printf("SECURE START: \n\r");
  uint32_t* current_address = (uint32_t*) NSEC_MEM_START;
  int count = 0;
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

	  //store the hash in the memory hashes buffer
	  for(int i = 0; i < 16; i++){
		  SEC_Mem_Hashes[count][i] = SEC_Mem_Digest[i];
	  }

	  //increment variables
	  count++;
	  current_address += BUFFER_SIZE/4;

  }


  //PRINT THE HASHES
//  printf("======================= FINAL HASH VALS =========================\n\r");
//  for(int j = 0; j< 256; j++){
//	  for(int i = 0; i<16; i++){
//	 	 //we'll print out bytes at a time
//	 	printf("%x", SEC_Mem_Hashes[j][i]);
//	  }
//	  printf("\n\r");
//  }

  /* USER CODE END 2 */

  /*************** Setup and jump to non-secure *******************************/

  NonSecure_Init();

  /* Non-secure software does not return, this code is not executed */
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
  * @brief  Non-secure call function
  *         This function is responsible for Non-secure initialization and switch
  *         to non-secure state
  * @retval None
  */
static void NonSecure_Init(void)
{
  funcptr_NS NonSecure_ResetHandler;

  SCB_NS->VTOR = VTOR_TABLE_NS_START_ADDR;

  /* Set non-secure main stack (MSP_NS) */
  __TZ_set_MSP_NS((*(uint32_t *)VTOR_TABLE_NS_START_ADDR));

  /* Get non-secure reset handler */
  NonSecure_ResetHandler = (funcptr_NS)(*((uint32_t *)((VTOR_TABLE_NS_START_ADDR) + 4U)));

  /* Start non-secure state software application */
  NonSecure_ResetHandler();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
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
  * @brief GTZC_S Initialization Function
  * @param None
  * @retval None
  */
static void MX_GTZC_S_Init(void)
{

  /* USER CODE BEGIN GTZC_S_Init 0 */

  /* USER CODE END GTZC_S_Init 0 */

  MPCBB_ConfigTypeDef MPCBB_NonSecureArea_Desc = {0};

  /* USER CODE BEGIN GTZC_S_Init 1 */

  /* USER CODE END GTZC_S_Init 1 */
  if (HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_IWDG, GTZC_TZSC_PERIPH_SEC|GTZC_TZSC_PERIPH_NPRIV) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_SPI3, GTZC_TZSC_PERIPH_SEC|GTZC_TZSC_PERIPH_NPRIV) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_USART1, GTZC_TZSC_PERIPH_SEC|GTZC_TZSC_PERIPH_NPRIV) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_HASH, GTZC_TZSC_PERIPH_SEC|GTZC_TZSC_PERIPH_NPRIV) != HAL_OK)
  {
    Error_Handler();
  }
  MPCBB_NonSecureArea_Desc.SecureRWIllegalMode = GTZC_MPCBB_SRWILADIS_ENABLE;
  MPCBB_NonSecureArea_Desc.InvertSecureState = GTZC_MPCBB_INVSECSTATE_NOT_INVERTED;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[0] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[1] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[2] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[3] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[4] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[5] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[6] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[7] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[8] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[9] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[10] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[11] =   0xFFFFFFFF;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[12] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[13] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[14] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[15] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[16] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[17] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[18] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[19] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[20] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[21] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[22] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[23] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_LockConfig_array[0] =   0x00000000;
  if (HAL_GTZC_MPCBB_ConfigMem(SRAM1_BASE, &MPCBB_NonSecureArea_Desc) != HAL_OK)
  {
    Error_Handler();
  }
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[0] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[1] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[2] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[3] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[4] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[5] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[6] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_SecConfig_array[7] =   0x00000000;
  MPCBB_NonSecureArea_Desc.AttributeConfig.MPCBB_LockConfig_array[0] =   0x00000000;
  if (HAL_GTZC_MPCBB_ConfigMem(SRAM2_BASE, &MPCBB_NonSecureArea_Desc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN GTZC_S_Init 2 */

  /* USER CODE END GTZC_S_Init 2 */

}

/**
  * @brief HASH Initialization Function
  * @param None
  * @retval None
  */
static void MX_HASH_Init(void)
{

  /* USER CODE BEGIN HASH_Init 0 */

  /* USER CODE END HASH_Init 0 */

  /* USER CODE BEGIN HASH_Init 1 */

  /* USER CODE END HASH_Init 1 */
  hhash.Init.DataType = HASH_DATATYPE_32B;
  if (HAL_HASH_Init(&hhash) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN HASH_Init 2 */

  /* USER CODE END HASH_Init 2 */

}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{

  /* USER CODE BEGIN ICACHE_Init 0 */

  /* USER CODE END ICACHE_Init 0 */

  /* USER CODE BEGIN ICACHE_Init 1 */

  /* USER CODE END ICACHE_Init 1 */

  /** Enable instruction cache in 1-way (direct mapped cache)
  */
  if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ICACHE_Init 2 */

  /* USER CODE END ICACHE_Init 2 */

}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */
//
  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */
//
  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg.Init.Window = 1100;
  hiwdg.Init.Reload = 1100;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */
//
  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_ODD;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  * Configure DMA for memory to memory transfers
  *   hdma_memtomem_dma1_channel1
  *   hdma_memtomem_dma1_channel4
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* Configure DMA request hdma_memtomem_dma1_channel1 on DMA1_Channel1 */
  hdma_memtomem_dma1_channel1.Instance = DMA1_Channel1;
  hdma_memtomem_dma1_channel1.Init.Request = DMA_REQUEST_MEM2MEM;
  hdma_memtomem_dma1_channel1.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma1_channel1.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma1_channel1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma1_channel1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_memtomem_dma1_channel1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_memtomem_dma1_channel1.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma1_channel1.Init.Priority = DMA_PRIORITY_LOW;
  if (HAL_DMA_Init(&hdma_memtomem_dma1_channel1) != HAL_OK)
  {
    Error_Handler( );
  }

  /*  */
  if (HAL_DMA_ConfigChannelAttributes(&hdma_memtomem_dma1_channel1, DMA_CHANNEL_NPRIV) != HAL_OK)
  {
    Error_Handler( );
  }

  /*  */
  if (HAL_DMA_ConfigChannelAttributes(&hdma_memtomem_dma1_channel1, DMA_CHANNEL_SEC) != HAL_OK)
  {
    Error_Handler( );
  }

  /*  */
  if (HAL_DMA_ConfigChannelAttributes(&hdma_memtomem_dma1_channel1, DMA_CHANNEL_SRC_NSEC) != HAL_OK)
  {
    Error_Handler( );
  }

  /*  */
  if (HAL_DMA_ConfigChannelAttributes(&hdma_memtomem_dma1_channel1, DMA_CHANNEL_DEST_SEC) != HAL_OK)
  {
    Error_Handler( );
  }

  /* Configure DMA request hdma_memtomem_dma1_channel4 on DMA1_Channel4 */
  hdma_memtomem_dma1_channel4.Instance = DMA1_Channel4;
  hdma_memtomem_dma1_channel4.Init.Request = DMA_REQUEST_MEM2MEM;
  hdma_memtomem_dma1_channel4.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma1_channel4.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma1_channel4.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma1_channel4.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_memtomem_dma1_channel4.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_memtomem_dma1_channel4.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma1_channel4.Init.Priority = DMA_PRIORITY_LOW;
  if (HAL_DMA_Init(&hdma_memtomem_dma1_channel4) != HAL_OK)
  {
    Error_Handler( );
  }

  /*  */
  if (HAL_DMA_ConfigChannelAttributes(&hdma_memtomem_dma1_channel4, DMA_CHANNEL_NPRIV) != HAL_OK)
  {
    Error_Handler( );
  }

  /*  */
  if (HAL_DMA_ConfigChannelAttributes(&hdma_memtomem_dma1_channel4, DMA_CHANNEL_SEC) != HAL_OK)
  {
    Error_Handler( );
  }

  /*  */
  if (HAL_DMA_ConfigChannelAttributes(&hdma_memtomem_dma1_channel4, DMA_CHANNEL_SRC_SEC) != HAL_OK)
  {
    Error_Handler( );
  }

  /*  */
  if (HAL_DMA_ConfigChannelAttributes(&hdma_memtomem_dma1_channel4, DMA_CHANNEL_DEST_SEC) != HAL_OK)
  {
    Error_Handler( );
  }

  /* DMA interrupt init */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_11|GPIO_PIN_10|GPIO_PIN_14|GPIO_PIN_15
                          |WHITE_LED_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(IR_SENSOR_PIN_GPIO_Port, IR_SENSOR_PIN_Pin, GPIO_PIN_NSEC);

  /*Configure GPIO pin : PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief  TxRx Transfer completed callback.
  * @param  hspi: SPI handle
  * @note   This example shows a simple way to report end of DMA TxRx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
//  printf("SPI Transfer complete.\n\r");
  wTransferState = TRANSFER_COMPLETE;
}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
//  printf("SPI transmission complete.\n\r");
  wTransferState = TRANSFER_COMPLETE;
}


void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
//  printf("SPI Reception complete.\n\r");
  wTransferState = TRANSFER_COMPLETE;
}


//redirects printf output to USART1 for displaying logs in a serial console
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART3 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}



//Returns position if s1 is substring of s2; -1 otherwise
int SearchForSig(uint8_t* signal, int sizeOfSig, uint8_t* data, int dataSize)
{
    int M = sizeOfSig;
    int N = dataSize;
    int retval = -1;


    /* A loop to slide signal one by one while comparing */
    for (int i = 0; i <= N - M; i++) {
      int j;

        //for current index i, check for signal match
      for (j = 0; j < M; j++){
        if (data[i + j] != signal[j]){
          break;
        }
      }
      if (j == M){
          retval = i;
          break;
      }
    }

    return retval;
}



//this function checks for & receives a classification from a stream of SPI data coming in through MISO
//It must be called in memory forensics after sending a full memory dump to the server
//void SECURE_SPI_Receive_Classification(){
//
//	int containsStartClassification;
//	int containsEndClassification;
//	//perform a new rx operation
//	wTransferState = TRANSFER_WAIT;
//	if (HAL_SPI_Receive_DMA(&hspi3, (uint8_t *)aRxBuffer, BUFFER_SIZE) != HAL_OK)
//	 {
//		 /* Transfer error in transmission process */
//		 printf("There was an error in starting up RX.\n\r");
//		 Error_Handler();
//	 }
//
//	 /*##-2- Wait for the end of the transfer ###################################*/
//	 while (wTransferState == TRANSFER_WAIT)
//	 {
//	 }
//
//	//check SPI transfer state
//	if(wTransferState == TRANSFER_COMPLETE){
//
//		//print the buffer received.
//		printf("Buffer received from SPI: \n\r");
//		for(int i = 0; i < BUFFER_SIZE; i++){
//			printf("%d\t", aRxBuffer[i]);
//		}
//		printf("\n\r");
//
//		//if the transfer completed, look for the start and end signals in the SPI rx buffer
//		containsStartClassification = SearchForSig(START_CLASSIFICATION_SIG, START_CLASSIFICATION_SIZE, aRxBuffer, BUFFER_SIZE);
//		containsEndClassification = SearchForSig(END_CLASSIFICATION_SIG, END_CLASSIFICATION_SIZE, aRxBuffer, BUFFER_SIZE);
//		if(containsStartClassification >= 0 && containsEndClassification >= 0){
//			//The start and end signals were found; make sure there's only one element between them
//			if(containsStartClassification == containsEndClassification - START_CLASSIFICATION_SIZE - 1){
//				//try to extract the classification
//				printf("Classification found. Extracting...\n\r");
//				int classificationInd = containsEndClassification - 1;
//				uint8_t classification = aRxBuffer[classificationInd];
//				printf("Classification: %c\n\r", classification);
//				if(classification == '1'){
//					printf("Memory was found to be benign.\n\r");
//				}else if(classification == '0'){
//					printf("Memory was found to be malicious.\n\r");
//				}else{
//					printf("ERROR. A classification other than 1 or 0 found!");
//				}
//			}else{
//				printf("Error: the SPI classification stream is incorrectly formatted.\n\r");
//				printf("Start classification: %d, End classification: %d\n\r", containsStartClassification, containsEndClassification);
//			}
//		}
//	}else{
//		//if transfer didn't complete
//		Error_Handler();
//	}
//
//}


//
////sends the start transmission signal out through the SPI Stream
////this allows the esp32 to know where memory dump data starts
//void SECURE_SPI_Send_Start_Signal(){
//	wTransferState = TRANSFER_WAIT;
//	uint8_t start_signal[] = "--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------<START_TRANSMISSION>";
//
//	printf("Sending the start transmission signal...\n\r");
//	if (HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *)start_signal, BUFFER_SIZE) != HAL_OK)
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
//
//
////sends the end transmission signal once a memory dump as fully gone out through SPI.
////this is used by the esp32 to know when a memory dump has been fully received
//void SECURE_SPI_Send_End_Signal(){
//	wTransferState = TRANSFER_WAIT;
//	uint8_t end_signal[] = "<END_TRANSMISSION>----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
//
//	printf("Sending the end transmission signal...\n\r");
//	if (HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *)end_signal, BUFFER_SIZE) != HAL_OK)
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
//
//
//
////Sends data that has been previously loaded into SEC_Mem_Buffer by a DMA transfer from NSC to SC
////this function must be called routinely until all memory buffers have been sent to the esp32
//void SECURE_SPI_Send_Data(){
//
//	wTransferState = TRANSFER_WAIT;
//	/*** send the current memory block from the non-secure flash bank to SPI ***/
//	if (HAL_SPI_Transmit_DMA(&hspi3, (uint8_t *) SEC_Mem_Buffer, BUFFER_SIZE) != HAL_OK)
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
//		   printf("There was an error in transferring the memory dump.\n\r");
//		   Error_Handler();
//		   break;
//	 }
//
//}
//
//
///**
//  * @brief  Secure service to toggle SPI communication on or off. Must be called @ beginning and end of SPI transmission
//  * @param state	SPI ON: 0 and SPI OFF: 1
//  * @retval SUCCESS or ERROR
//  */
//void SECURE_SPI_Toggle_Comm(int state){
//	if(state > 0){
//		//off state
//		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
//
//	}else{
//		//on state
//		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
//	}
//}



//used in non-secure environment to check the secure memory buffer for successful transfer.
void SECURE_Print_Mem_Buffer(uint8_t* buf, int size)
{

  //print the contents of the first transfer
	 for(int i = 0; i<size; i++){
//		 if((i+1)%16==1){
//			 printf("%08p|\t", addr);
//		 }
		 //we'll print out bytes at a time
		printf("0x%02x\t", buf[i]);
		if((i+1)%16==0){
			printf("\n\r");
		}
//		if((i+1)%4==0){
//			addr+=1;
//		}
	 }

}


//callback functions for nonsecure --> secure mem transfer
static void NonsecureToSecureTransferComplete(DMA_HandleTypeDef *hdma_memtomem_dma1_channel1)
{
//	printf("nonsecure to secure transf complete executed.\n\r");
  NonsecureToSecureTransferCompleteDetected = 1;
}


/**
  * @brief  DMA conversion error callback
  * @note   This function is executed when the transfer error interrupt
  *         is generated during DMA transfer
  * @retval None
  */
static void NonsecureToSecureTransferError(DMA_HandleTypeDef *hdma_memtomem_dma1_channel1)
{
  NonsecureToSecureTransferErrorDetected = 1;
}


/**
  * @brief  DMA conversion complete callback
  * @note   This function is executed when the transfer complete interrupt
  *         is generated
  * @retval None
  */
static void SecureToSecureTransferComplete(DMA_HandleTypeDef *hdma_memtomem_dma1_channel4)
{
  SecureToSecureTransferCompleteDetected = 1;
}

/**
  * @brief  DMA conversion error callback
  * @note   This function is executed when the transfer error interrupt
  *         is generated during DMA transfer
  * @retval None
  */
static void SecureToSecureTransferError(DMA_HandleTypeDef *hdma_memtomem_dma1_channel4)
{
  SecureToSecureTransferErrorDetected = 1;
}



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
