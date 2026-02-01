/**
  ******************************************************************************
  * @file    stm32l475e_iot01_hsensor.c
  * @author  MCD Application Team
  * @brief   BSP Humidity Sensor (HTS221)
  ******************************************************************************
  */

#include "stm32l475e_iot01_hsensor.h"

/* Private variable ----------------------------------------------------------*/
static HSENSOR_DrvTypeDef *hsensor_drv;

/**
  * @brief  Initialize humidity sensor
  */
uint32_t BSP_HSENSOR_Init(void)
{
  hsensor_drv = &HTS221_H_Drv;

  SENSOR_IO_Init();
  hsensor_drv->Init(TSENSOR_I2C_ADDRESS);

  return HSENSOR_OK;
}

/**
  * @brief  Read relative humidity (%)
  */
float BSP_HSENSOR_ReadHumidity(void)
{
  return hsensor_drv->ReadHumidity(TSENSOR_I2C_ADDRESS);
}
