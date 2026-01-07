#ifndef ETH_INIT_H
#define ETH_INIT_H

#include "stm32n6xx_hal.h"

#define ETH_RX_BUFFER_SIZE 1536U
#define ETH_TX_BUFFER_SIZE 1536U

extern ETH_HandleTypeDef heth1;

void MX_ETH1_Init(void);
void ETH_PHY_Init(void);
uint32_t ETH_PHY_GetLinkState(void);

#endif /* ETH_INIT_H */
