/**
******************************************************************************
* @file    stm32n6xx_hal_msp.c
* @author  GPM Application Team
*
******************************************************************************
* @attention
*
* Copyright (c) 2024 STMicroelectronics.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
*/
#include "main.h"

void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
}

void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
  GPIO_InitTypeDef GPIO_Init;
  GPIO_DelayTypeDef delay_conf;

  UNUSED(heth);

  __HAL_RCC_ETH1MAC_CLK_ENABLE();
  __HAL_RCC_ETH1TX_CLK_ENABLE();
  __HAL_RCC_ETH1RX_CLK_ENABLE();

  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  GPIO_Init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_Init.Mode = GPIO_MODE_AF_PP;
  GPIO_Init.Pull = GPIO_NOPULL;
  GPIO_Init.Alternate = GPIO_AF11_ETH1;
  GPIO_Init.Pin = GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_12;
  HAL_GPIO_Init(GPIOD, &GPIO_Init);

  GPIO_Init.Pin = GPIO_PIN_2  | GPIO_PIN_7  | GPIO_PIN_5  |
                  GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_11 |
                  GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOF, &GPIO_Init);

  delay_conf.Delay = GPIO_DELAY_PS_500;
  delay_conf.Path = GPIO_PATH_IN;
  HAL_GPIO_SetDelay(GPIOF, GPIO_PIN_7, &delay_conf);

  GPIO_Init.Pin = GPIO_PIN_3 | GPIO_PIN_4;
  HAL_GPIO_Init(GPIOG, &GPIO_Init);

  GPIO_Init.Speed = GPIO_SPEED_FREQ_MEDIUM;
  GPIO_Init.Pin =  GPIO_PIN_0;
  GPIO_Init.Alternate = GPIO_AF12_ETH1;
  HAL_GPIO_Init(GPIOF, &GPIO_Init);

  HAL_NVIC_SetPriority(ETH1_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(ETH1_IRQn);
}

void HAL_ETH_MspDeInit(ETH_HandleTypeDef* heth)
{
  UNUSED(heth);

  __HAL_RCC_ETH1MAC_CLK_DISABLE();
  __HAL_RCC_ETH1TX_CLK_DISABLE();
  __HAL_RCC_ETH1RX_CLK_DISABLE();

  HAL_GPIO_DeInit(GPIOF,  GPIO_PIN_4|GPIO_PIN_7|GPIO_PIN_10|GPIO_PIN_11|
                           GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);

  HAL_GPIO_DeInit(GPIOG, GPIO_PIN_11);

  HAL_NVIC_DisableIRQ(ETH1_IRQn);
}
