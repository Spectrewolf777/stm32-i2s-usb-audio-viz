/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// Define buffer size. 4096 half-words = 8192 bytes.
// It must be an even number so it can be split into two perfect halves.
#define DAC3100_I2C_ADDR (0x18 << 1) // Default I2C address
#define AUDIO_BUFFER_SIZE 4096
const int GAIN_FACTOR = 1;
// 16-bit (Half-Word) buffers for Circular DMA
uint16_t rxBuffer[AUDIO_BUFFER_SIZE];
uint16_t txBuffer[AUDIO_BUFFER_SIZE];

// Flags to handle processing in the main loop instead of the ISR
volatile uint8_t processHalf1 = 0;
volatile uint8_t processHalf2 = 0;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

int __io_putchar(int ch) {


    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}


static uint8_t current_page = 0xFF;
uint8_t TxBuffer[] = "Hello World! From STM32 USB CDC Device To Virtual COM Port\r\n";
uint8_t TxBufferLen = sizeof(TxBuffer);

void USB_CDC_RxHandler(uint8_t* Buf, uint32_t Len)
{
	CDC_Transmit_FS(Buf, Len);
}



void FirstHalfProcessAudio()
{


	 for(int i = 0; i < (AUDIO_BUFFER_SIZE / 2); i++)
		          {
		              // Cast to signed 16-bit! Audio is 2's complement PCM.
		              int32_t sample = (int16_t)rxBuffer[i];

		              sample *= GAIN_FACTOR; // Boost the volume

		              // 3. Hard Clipping (Prevent overflow static/screeching)
		              if (sample > 32767)  sample = 32767;
		              if (sample < -32768) sample = -32768;

		              txBuffer[i] = (uint16_t)sample;
		          }

}


void SecondHalfProcessAudio()
{


	for(int i = (AUDIO_BUFFER_SIZE / 2); i < AUDIO_BUFFER_SIZE; i++)
		          {
		              int32_t sample = (int16_t)rxBuffer[i];

		              sample *= GAIN_FACTOR;

		              if (sample > 32767)  sample = 32767;
		              if (sample < -32768) sample = -32768;

		              txBuffer[i] = (uint16_t)sample;
		          }



}








void DAC3100_SetPage(I2C_HandleTypeDef *hi2c, uint8_t page)
{
    if (page != current_page) {
        uint8_t buffer[2] = {0x00, page}; // Register 0 is always Page Select
        HAL_I2C_Master_Transmit(hi2c, DAC3100_I2C_ADDR, buffer, 2, 100);
        current_page = page;
    }
}







void DAC3100_WriteRegister(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data)
{
    uint8_t buffer[2] = {reg, data};
    HAL_I2C_Master_Transmit(hi2c, DAC3100_I2C_ADDR, buffer, 2, 100);
}

void DAC3100_Init(I2C_HandleTypeDef *hi2c)
{
    // 1. Define starting point
    DAC3100_SetPage(hi2c, 0);       // Ensure we are on Page 0
    DAC3100_WriteRegister(hi2c, 0x01, 0x01); // SW Reset
    HAL_Delay(10);

    // 2. Program clock settings
    // All these are on Page 0, so no need to call SetPage again!
    DAC3100_WriteRegister(hi2c, 0x04, 0x03); // Clock mux
    DAC3100_WriteRegister(hi2c, 0x06, 0x08); // J value
    DAC3100_WriteRegister(hi2c, 0x07, 0x00); // D MSB
    DAC3100_WriteRegister(hi2c, 0x08, 0x00); // D LSB
    DAC3100_WriteRegister(hi2c, 0x05, 0x91); // Power up PLL

    DAC3100_WriteRegister(hi2c, 0x0B, 0x88); // Program and power up NDAC
    DAC3100_WriteRegister(hi2c, 0x0C, 0x82); // # MDAC is powered up and set to 2

    DAC3100_WriteRegister(hi2c, 0x0D, 0x00); // # DOSR = 128, DOSR(9:8) = 0, DOSR(7:0) = 128
    DAC3100_WriteRegister(hi2c, 0x0E, 0x80); // #

    DAC3100_WriteRegister(hi2c, 0x1B, 0x00); // #

    DAC3100_WriteRegister(hi2c, 0x3C, 0x0B); // #
    DAC3100_WriteRegister(hi2c, 0x00, 0x08); // #
    DAC3100_WriteRegister(hi2c, 0x01, 0x04); // #
    DAC3100_WriteRegister(hi2c, 0x00, 0x00); // #

    DAC3100_WriteRegister(hi2c, 0x74, 0x00); // #

    DAC3100_SetPage(hi2c,1); //set page 1

    DAC3100_WriteRegister(hi2c, 0x1F, 0x04); // #

    DAC3100_WriteRegister(hi2c, 0x21, 0x4E); // #headphones

    DAC3100_WriteRegister(hi2c, 0x23, 0x44); // #headphones route

    DAC3100_WriteRegister(hi2c, 0x28, 0x06); // #headphones
    DAC3100_WriteRegister(hi2c, 0x29, 0x06); // #headphones

    DAC3100_WriteRegister(hi2c, 0x2A, 0x1C);

    DAC3100_WriteRegister(hi2c, 0x1F, 0xC2);
    DAC3100_WriteRegister(hi2c, 0x20, 0x86);
    DAC3100_WriteRegister(hi2c, 0x24, 0x92);
    DAC3100_WriteRegister(hi2c, 0x25, 0x92);
    DAC3100_WriteRegister(hi2c, 0x26, 0x92);

    DAC3100_SetPage(hi2c,0); //set page 0

    DAC3100_WriteRegister(hi2c, 0x3F, 0xD4);
    DAC3100_WriteRegister(hi2c, 0x41, 0xD4);
    DAC3100_WriteRegister(hi2c, 0x42, 0xD4);
    DAC3100_WriteRegister(hi2c, 0x40, 0x00);



}

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S1_Init();
  MX_I2S2_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(3100);
  CDC_Transmit_FS(TxBuffer, TxBufferLen);


  // Clear buffers to avoid initial noise/pops
    memset(rxBuffer, 0, sizeof(rxBuffer));
    memset(txBuffer, 0, sizeof(txBuffer));

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(300);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_SET);


    DAC3100_Init(&hi2c1);

    // 1. Start I2S2 Transmit (MAX98357A) in Circular DMA mode
    // We start TX first so it outputs silence while waiting for the first RX packet
    HAL_I2S_Transmit_DMA(&hi2s2, txBuffer, AUDIO_BUFFER_SIZE);

    // 2. Start I2S1 Receive (ADAU7002) in Circular DMA mode
    HAL_I2S_Receive_DMA(&hi2s1, rxBuffer, AUDIO_BUFFER_SIZE);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  // Process the First Half of the buffer
	  if (processHalf1 == 1)
	      {




	          // Apply digital gain
		  FirstHalfProcessAudio();


		  // --- SEND TO PYTHON ---
		          // We send the first half of txBuffer (AUDIO_BUFFER_SIZE / 2 samples)
		          // Each sample is 2 bytes, so we multiply by 2 for the byte length.
		   CDC_Transmit_FS((uint8_t*)&txBuffer[0], (AUDIO_BUFFER_SIZE / 2) * 2);




	          processHalf1 = 0; // Clear flag
	      }

	      // Process the Second Half of the buffer
	      if (processHalf2 == 1)
	      {




	    	  SecondHalfProcessAudio();
	    	  CDC_Transmit_FS((uint8_t*)&txBuffer[AUDIO_BUFFER_SIZE / 2], (AUDIO_BUFFER_SIZE / 2) * 2);



	          processHalf2 = 0; // Clear flag
	      }  }
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 24;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 14;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOMEDIUM;
  RCC_OscInitStruct.PLL.PLLFRACN = 3072;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI1) // Ensure it's I2S1 (ADAU7002)
    {
        // Set flag to process the first half of the buffer in the main loop
        processHalf1 = 1;
    }
}

/**
  * @brief  Rx Transfer completed callbacks.
  * @param  hi2s: pointer to a I2S_HandleTypeDef structure.
  */
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI1)
    {
        // Set flag to process the second half of the buffer in the main loop
        processHalf2 = 1;
    }
}
/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
#ifdef USE_FULL_ASSERT
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
