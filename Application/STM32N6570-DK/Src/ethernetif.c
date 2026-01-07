#include "ethernetif.h"

#include <string.h>

#include "eth_init.h"
#include "lwip/etharp.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"
#include "stm32n6xx_hal.h"

#define ETH_RX_POOL_SIZE ETH_RX_DESC_CNT

static uint8_t rx_pool[ETH_RX_POOL_SIZE][ETH_RX_BUFFER_SIZE]
  __attribute__((section(".eth_rx_buf"), aligned(32)));
static uint32_t rx_pool_idx;

static uint8_t tx_buffer[ETH_TX_BUFFER_SIZE] __attribute__((aligned(32)));

void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
  (void)buff;
}

static void cache_clean(const void *addr, uint32_t len)
{
  uint32_t start = ((uint32_t)addr) & ~31U;
  uint32_t end = (uint32_t)addr + len;
  uint32_t size = end - start;
  SCB_CleanDCache_by_Addr((uint32_t *)start, (int32_t)size);
}

static void cache_invalidate(const void *addr, uint32_t len)
{
  uint32_t start = ((uint32_t)addr) & ~31U;
  uint32_t end = (uint32_t)addr + len;
  uint32_t size = end - start;
  SCB_InvalidateDCache_by_Addr((uint32_t *)start, (int32_t)size);
}

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  ETH_TxPacketConfigTypeDef tx_config;
  ETH_BufferTypeDef tx_buffer_desc;

  (void)netif;

  if (p->tot_len > sizeof(tx_buffer))
  {
    return ERR_MEM;
  }

  pbuf_copy_partial(p, tx_buffer, p->tot_len, 0);
  cache_clean(tx_buffer, p->tot_len);

  memset(&tx_config, 0, sizeof(tx_config));
  tx_config.Length = p->tot_len;
  tx_config.TxBuffer = &tx_buffer_desc;
  tx_config.Attributes = ETH_TX_PACKETS_FEATURES_CRCPAD;
  tx_config.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  tx_config.ChecksumCtrl = ETH_CHECKSUM_DISABLE;

  tx_buffer_desc.buffer = tx_buffer;
  tx_buffer_desc.len = p->tot_len;
  tx_buffer_desc.next = NULL;

  if (HAL_ETH_Transmit(&heth1, &tx_config, 1000U) != HAL_OK)
  {
    return ERR_IF;
  }

  /* Release transmitted descriptors to avoid TX stall after a few packets. */
  (void)HAL_ETH_ReleaseTxPacket(&heth1);

  return ERR_OK;
}

static void low_level_init(struct netif *netif)
{
  MX_ETH1_Init();
  ETH_PHY_Init();
  HAL_ETH_Start(&heth1);

  netif->hwaddr_len = ETH_HWADDR_LEN;
  memcpy(netif->hwaddr, heth1.Init.MACAddr, ETH_HWADDR_LEN);
  netif->mtu = 1500;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
}

err_t ethernetif_init(struct netif *netif)
{
  if (netif == NULL)
  {
    return ERR_ARG;
  }

  netif->name[0] = 's';
  netif->name[1] = 't';
  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  low_level_init(netif);
  return ERR_OK;
}

void ethernetif_input(struct netif *netif)
{
  struct pbuf *p = NULL;

  while (HAL_ETH_ReadData(&heth1, (void **)&p) == HAL_OK)
  {
    if (p != NULL)
    {
      if (netif->input(p, netif) != ERR_OK)
      {
        pbuf_free(p);
      }
    }
    p = NULL;
  }
}

void ethernetif_poll_link(struct netif *netif)
{
  static uint32_t last_check_ms = 0U;
  uint32_t now = HAL_GetTick();

  if ((now - last_check_ms) < 500U)
  {
    return;
  }
  last_check_ms = now;

  if (ETH_PHY_GetLinkState() != 0U)
  {
    netif_set_link_up(netif);
  }
  else
  {
    netif_set_link_down(netif);
  }
}

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
  *buff = rx_pool[rx_pool_idx];
  rx_pool_idx = (rx_pool_idx + 1U) % ETH_RX_POOL_SIZE;
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
  struct pbuf *p = NULL;

  if (buff == NULL || Length == 0U)
  {
    return;
  }

  cache_invalidate(buff, Length);
  p = pbuf_alloc(PBUF_RAW, Length, PBUF_POOL);
  if (p == NULL)
  {
    return;
  }

  pbuf_take(p, buff, Length);

  if (*pStart == NULL)
  {
    *pStart = p;
    *pEnd = p;
  }
  else
  {
    pbuf_cat((struct pbuf *)(*pEnd), p);
    *pEnd = p;
  }
}
