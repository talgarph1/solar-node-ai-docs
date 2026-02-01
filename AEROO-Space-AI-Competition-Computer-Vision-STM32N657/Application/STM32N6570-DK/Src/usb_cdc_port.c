#include "ux_api.h"
#include "stm32n6xx_hal.h"

ALIGN_TYPE _ux_utility_interrupt_disable(void)
{
  ALIGN_TYPE primask = __get_PRIMASK();
  __disable_irq();
  return primask;
}

VOID _ux_utility_interrupt_restore(ALIGN_TYPE flags)
{
  if (flags == 0U)
  {
    __enable_irq();
  }
  else
  {
    __disable_irq();
  }
}

ULONG _ux_utility_time_get(void)
{
  return (ULONG)HAL_GetTick();
}
