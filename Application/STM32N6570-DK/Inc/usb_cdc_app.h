#ifndef USB_CDC_APP_H
#define USB_CDC_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "od_pp_output_if.h"

void USB_CDC_Init(void);
void USB_CDC_Tasks(void);
void USB_CDC_SendDetections(const od_pp_out_t *p_postprocess, uint32_t inference_ms);

#ifdef __cplusplus
}
#endif

#endif /* USB_CDC_APP_H */
