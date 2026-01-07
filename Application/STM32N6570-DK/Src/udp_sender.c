#include "udp_sender.h"

#include <string.h>
#include <stdio.h>

#include "app_config.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

static struct udp_pcb *g_pcb;
static ip4_addr_t g_remote_ip;
static uint16_t g_remote_port;
static uint32_t g_last_send_ms;
extern const char *classes_table[NB_CLASSES];

void udp_sender_init(const ip4_addr_t *remote_ip, uint16_t remote_port)
{
  if (remote_ip != NULL)
  {
    g_remote_ip = *remote_ip;
  }
  g_remote_port = remote_port;
  g_pcb = udp_new_ip_type(IPADDR_TYPE_V4);
  g_last_send_ms = 0U;
}

void udp_sender_send_detections(const od_pp_out_t *p_postprocess, uint32_t inference_ms)
{
  char buffer[512];
  struct pbuf *p;
  size_t used;
  int i;
  uint32_t now;

  if (g_pcb == NULL || p_postprocess == NULL)
  {
    return;
  }
  if (p_postprocess->nb_detect <= 0)
  {
    return;
  }

  now = HAL_GetTick();
  if ((now - g_last_send_ms) < 50U)
  {
    return;
  }
  g_last_send_ms = now;

  used = (size_t)snprintf(buffer, sizeof(buffer),
                          "{\"t\":%lu,\"n\":%ld,\"inf_ms\":%lu,\"objs\":[",
                          (unsigned long)now,
                          (long)p_postprocess->nb_detect,
                          (unsigned long)inference_ms);

  for (i = 0; i < p_postprocess->nb_detect && used < (sizeof(buffer) - 32U); i++)
  {
    const od_pp_outBuffer_t *roi = &p_postprocess->pOutBuff[i];
    const char *label = "unknown";
    if (roi->class_index >= 0 && roi->class_index < NB_CLASSES)
    {
      label = classes_table[roi->class_index];
    }
    int written = snprintf(buffer + used, sizeof(buffer) - used,
                           "%s{\"c\":%ld,\"label\":\"%s\",\"p\":%.3f,\"x\":%.3f,\"y\":%.3f,\"w\":%.3f,\"h\":%.3f}",
                           (i == 0) ? "" : ",",
                           (long)roi->class_index,
                           label,
                           (double)roi->conf,
                           (double)roi->x_center,
                           (double)roi->y_center,
                           (double)roi->width,
                           (double)roi->height);
    if (written < 0)
    {
      break;
    }
    used += (size_t)written;
  }

  if (used < (sizeof(buffer) - 4U))
  {
    buffer[used++] = ']';
    buffer[used++] = '}';
    buffer[used++] = '\n';
    buffer[used] = '\0';
  }

  p = pbuf_alloc(PBUF_TRANSPORT, (u16_t)used, PBUF_RAM);
  if (p == NULL)
  {
    return;
  }
  memcpy(p->payload, buffer, used);
  udp_sendto(g_pcb, p, (const ip_addr_t *)&g_remote_ip, g_remote_port);
  pbuf_free(p);
}
