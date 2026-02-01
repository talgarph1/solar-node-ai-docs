#include "lwip/arch.h"
#include "stm32n6xx_hal.h"

uint32_t sys_now(void)
{
  return HAL_GetTick();
}
