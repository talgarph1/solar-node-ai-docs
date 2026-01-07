#include "eth_init.h"

#define PHY_BCR                  0x00U
#define PHY_BSR                  0x01U
#define PHY_ID1                  0x02U
#define PHY_ID2                  0x03U
#define PHY_BCR_RESET            0x8000U
#define PHY_BCR_AUTONEG          0x1200U
#define PHY_BSR_LINK_STATUS      0x0004U
#define PHY_BSR_AUTONEG_DONE     0x0020U

static uint32_t g_phy_addr = 0xFFFFFFFFU;

ETH_HandleTypeDef heth1;

ETH_DMADescTypeDef DMARxDscrTab[ETH_DMA_RX_CH_CNT][ETH_RX_DESC_CNT]
  __attribute__((section(".eth_desc"), aligned(32)));
ETH_DMADescTypeDef DMATxDscrTab[ETH_DMA_TX_CH_CNT][ETH_TX_DESC_CNT]
  __attribute__((section(".eth_desc"), aligned(32)));

static uint8_t g_mac_addr[6] = { 0x00, 0x80, 0xE0, 0x00, 0x10, 0x00 };

void MX_ETH1_Init(void)
{
  heth1.Instance = ETH1;
  heth1.Init.MACAddr = g_mac_addr;
  heth1.Init.MediaInterface = HAL_ETH_RGMII_MODE;
  heth1.Init.RxBuffLen = ETH_RX_BUFFER_SIZE;

  for (int ch = 0; ch < ETH_DMA_CH_CNT; ch++)
  {
    heth1.Init.TxDesc[ch] = DMATxDscrTab[ch];
    heth1.Init.RxDesc[ch] = DMARxDscrTab[ch];
  }

  HAL_ETH_Init(&heth1);
}

void ETH_PHY_Init(void)
{
  uint32_t reg;
  uint32_t tick_start = HAL_GetTick();

  if (g_phy_addr == 0xFFFFFFFFU)
  {
    for (uint32_t addr = 0; addr < 32U; addr++)
    {
      if (HAL_ETH_ReadPHYRegister(&heth1, addr, PHY_ID1, &reg) != HAL_OK)
      {
        continue;
      }
      if (reg != 0x0000U && reg != 0xFFFFU)
      {
        g_phy_addr = addr;
        break;
      }
    }
  }

  if (g_phy_addr == 0xFFFFFFFFU)
  {
    g_phy_addr = 0U;
  }

  HAL_ETH_WritePHYRegister(&heth1, g_phy_addr, PHY_BCR, PHY_BCR_RESET);
  do
  {
    HAL_ETH_ReadPHYRegister(&heth1, g_phy_addr, PHY_BCR, &reg);
    if ((HAL_GetTick() - tick_start) > 1000U)
    {
      return;
    }
  } while ((reg & PHY_BCR_RESET) != 0U);

  HAL_ETH_WritePHYRegister(&heth1, g_phy_addr, PHY_BCR, PHY_BCR_AUTONEG);

  tick_start = HAL_GetTick();
  do
  {
    HAL_ETH_ReadPHYRegister(&heth1, g_phy_addr, PHY_BSR, &reg);
    if ((HAL_GetTick() - tick_start) > 3000U)
    {
      break;
    }
  } while ((reg & PHY_BSR_AUTONEG_DONE) == 0U);
}

uint32_t ETH_PHY_GetLinkState(void)
{
  uint32_t reg = 0;
  if (g_phy_addr == 0xFFFFFFFFU)
  {
    return 0U;
  }
  HAL_ETH_ReadPHYRegister(&heth1, g_phy_addr, PHY_BSR, &reg);
  return (reg & PHY_BSR_LINK_STATUS);
}
