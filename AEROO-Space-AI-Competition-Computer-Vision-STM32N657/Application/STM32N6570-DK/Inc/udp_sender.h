#ifndef UDP_SENDER_H
#define UDP_SENDER_H

#include "lwip/ip4_addr.h"
#include "app_postprocess.h"

void udp_sender_init(const ip4_addr_t *remote_ip, uint16_t remote_port);
void udp_sender_send_detections(const od_pp_out_t *p_postprocess, uint32_t inference_ms);

#endif /* UDP_SENDER_H */
