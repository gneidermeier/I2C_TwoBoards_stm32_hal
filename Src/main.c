/**
  ******************************************************************************
  * @file    I2C/I2C_TwoBoards_ComPolling/Src/main.c 
  * @author  MCD Application Team
  * @brief   This sample code shows how to use STM32F3xx I2C HAL API to transmit
  *          and receive a data buffer with a communication process based on
  *          Polling transfer.
  *          The communication is done using 2 Boards.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "main.h"

/** @addtogroup STM32F3xx_HAL_Examples
  * @{
  */

/** @addtogroup I2C_TwoBoards_ComPolling
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Uncomment this line to use the board as master, if not it is used as slave */
#define MASTER_BOARD  // 1
#define I2C_ADDRESS        65

/* I2C TIMING Register define when I2C clock source is SYSCLK */
/* I2C TIMING is calculated in case of the I2C Clock source is the SYSCLK = 64 MHz */
/* This example use TIMING to 0x00400B27 to reach 1 MHz speed (Rise time = 26ns, Fall time = 2ns) */
#define I2C_TIMING      0x00400B27

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* I2C handler declaration */
I2C_HandleTypeDef I2cHandle;


/* Buffer used for transmission */
uint8_t aTxBuffer[] = " ****I2C_TwoBoards communication based on Polling****";

/* Buffer used for reception */
uint8_t aRxBuffer[RXBUFFERSIZE];

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static uint16_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint16_t BufferLength);
static void Error_Handler(void);

static void MX_USART2_UART_Init(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
    char uart_out[128];

  /* STM32F3xx HAL library initialization:
       - Configure the Flash prefetch
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 64 MHz */
  SystemClock_Config();

  MX_USART2_UART_Init();

  /* Configure LED2 */
  BSP_LED_Init(LED2);


  /*##-1- Configure the I2C peripheral ######################################*/
  I2cHandle.Instance             = I2Cx;
  I2cHandle.Init.Timing          = I2C_TIMING;
  I2cHandle.Init.OwnAddress1     = I2C_ADDRESS;
  I2cHandle.Init.AddressingMode  = I2C_ADDRESSINGMODE_10BIT;
  I2cHandle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  I2cHandle.Init.OwnAddress2     = 0xFF;
  I2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  I2cHandle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
  
  if(HAL_I2C_Init(&I2cHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Enable the Analog I2C Filter */
  HAL_I2CEx_ConfigAnalogFilter(&I2cHandle,I2C_ANALOGFILTER_ENABLE);

  /* Infinite loop */
  while (1)
  {
      //      BSP_LED_Toggle(LED2);
#ifdef MASTER_BOARD

  sprintf(uart_out, "\r\nTransmitting to Peripheral (10 sec timeout)\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t *)uart_out, strlen(uart_out), 100);

  /* The board sends the message and expects to receive it back */
  /* Turn LED2 off: Transfer in process */
  BSP_LED_Off(LED2);

  /*##-2- Start the transmission process #####################################*/  
  /* While the I2C in reception process, user can transmit data through 
     "aTxBuffer" buffer */

  /* Timeout is set to 10S */
  while(HAL_I2C_Master_Transmit(
          &I2cHandle, (uint16_t)I2C_ADDRESS, (uint8_t*)aTxBuffer, TXBUFFERSIZE, 10000)!= HAL_OK)
  {
    /* Error_Handler() function is called when Timeout error occurs.
       When Acknowledge failure occurs (Slave don't acknowledge its address)
       Master restarts communication */
    if (HAL_I2C_GetError(&I2cHandle) == HAL_I2C_ERROR_AF)
    {
        sprintf(uart_out, "Error HAL_I2C_ERROR_AF (address doesn't match)\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t *)uart_out, strlen(uart_out), 100);
    }
    else
    {
        sprintf(uart_out, "Timeout or other error\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t *)uart_out, strlen(uart_out), 100);
    }
    HAL_Delay(1000);
    Error_Handler();
  }

  sprintf(uart_out, "Waiting response from Peripheral (10 sec timeout)\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t *)uart_out, strlen(uart_out), 100);

  /*##-3- Put I2C peripheral in reception process ############################*/ 
  /* Timeout is set to 10S */ 
  while(HAL_I2C_Master_Receive(&I2cHandle, (uint16_t)I2C_ADDRESS, (uint8_t *)aRxBuffer, RXBUFFERSIZE, 10000) != HAL_OK)
  {
    /* Error_Handler() function is called when Timeout error occurs.
       When Acknowledge failure occurs (Slave don't acknowledge it's address)
       Master restarts communication */
    if (HAL_I2C_GetError(&I2cHandle) != HAL_I2C_ERROR_AF)
    {
      Error_Handler();
    }
  }

  /* Turn LED2 on: Transfer complete */
  BSP_LED_On(LED2);

  sprintf(uart_out, "HAL_I2C_Master_Receive success, response from Peripheral: \"%s\".\r\n", aRxBuffer);
  HAL_UART_Transmit(&huart2, (uint8_t *)uart_out, strlen(uart_out), 100);

  /*##-4- Compare the sent and received buffers ##############################*/
  if(Buffercmp((uint8_t*)aTxBuffer,(uint8_t*)aRxBuffer,RXBUFFERSIZE))
  {
    /* Processing Error */
    Error_Handler();
  }

  HAL_Delay(1000);

#else // peripheral

  sprintf(uart_out, "\r\nPeripheral Receive ...\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t *)uart_out, strlen(uart_out), 100);

  /* The board receives the message and sends it back */

  /*##-2- Put I2C peripheral in reception process ############################*/
  uint32_t Timeout = 1000 * 30; /* Timeout is set to ... 10000? */
  if(HAL_I2C_Slave_Receive(&I2cHandle, (uint8_t *)aRxBuffer, RXBUFFERSIZE, Timeout) != HAL_OK)
  {
    uint32_t error = HAL_I2C_GetError(&I2cHandle);
    //if (HAL_I2C_GetError(&I2cHandle) == HAL_I2C_ERROR_AF)
    {
        sprintf(uart_out, "I2C_Slave_Receive error (%d)\r\n", error);
        HAL_UART_Transmit(&huart2, (uint8_t *)uart_out, strlen(uart_out), 100);
    }
    /* Transfer error in reception process */
    Error_Handler();
  }

  /* Turn LED2 on: Transfer in process */
  BSP_LED_On(LED2);

  sprintf(uart_out, "... received \"%s\", responding.\r\n", aRxBuffer);
  HAL_UART_Transmit(&huart2, (uint8_t *)uart_out, strlen(uart_out), 100);

// controller expects verbatim response so we send the RxBuffer we just received

  /*##-3- Start the transmission process #####################################*/
  /* While the I2C in reception process, user can transmit data through "aTxBuffer" buffer */
  /* Timeout is set to 10S */
  if(HAL_I2C_Slave_Transmit(
      &I2cHandle, (uint8_t*)aRxBuffer /* aTxBuffer */, /* TXBUFFERSIZE */ strlen((const char *)aRxBuffer), 10000)!= HAL_OK)
  {
    /* Transfer error in transmission process */
    Error_Handler();
  }

  /* Turn LED2 off: Transfer in transmission process is correct */
  BSP_LED_Off(LED2);

  sprintf(uart_out, "I2C_Slave_Transmit success.\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t *)uart_out, strlen(uart_out), 100);
  
#endif /* MASTER_BOARD */
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSI)
  *            SYSCLK(Hz)                     = 64000000
  *            HCLK(Hz)                       = 64000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            HSI Frequency(Hz)              = 8000000
  *            PREDIV                         = RCC_PREDIV_DIV2 (2)
  *            PLLMUL                         = RCC_PLL_MUL16 (16)
  *            Flash Latency(WS)              = 2
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

  /* HSI Oscillator already ON after system reset, activate PLL with HSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 38400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  I2C error callbacks.
  * @param  I2cHandle: I2C handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *I2cHandle)
{
  /** Error_Handler() function is called when error occurs.
    * 1- When Slave don't acknowledge it's address, Master restarts communication.
    * 2- When Master don't acknowledge the last data transferred, Slave don't care in this example.
    */
  if (HAL_I2C_GetError(I2cHandle) != HAL_I2C_ERROR_AF)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  char uart_out[128];
  sprintf(uart_out, "Error_Handler ... reset.\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t *)uart_out, strlen(uart_out), 100);

  while(1)
  {
      NVIC_SystemReset();
  } 
}

/**
  * @brief  Compares two buffers.
  * @param  pBuffer1, pBuffer2: buffers to be compared.
  * @param  BufferLength: buffer's length
  * @retval 0  : pBuffer1 identical to pBuffer2
  *         >0 : pBuffer1 differs from pBuffer2
  */
static uint16_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint16_t BufferLength)
{
  while (BufferLength--)
  {
    if ((*pBuffer1) != *pBuffer2)
    {
      return BufferLength;
    }
    pBuffer1++;
    pBuffer2++;
  }

  return 0;
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif


/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
