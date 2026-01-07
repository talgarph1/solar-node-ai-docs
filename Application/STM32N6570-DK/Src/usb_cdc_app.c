#include "usb_cdc_app.h"

#include <string.h>
#include <stdio.h>

#include "stm32n6xx_hal.h"
#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"
#include "ux_dcd_stm32.h"
#include "ux_system.h"
#include "stm32n6570_discovery.h"

#define USBX_MEM_POOL_SIZE (32U * 1024U)
#define USB_CDC_TX_INTERVAL_MS 50U

static UCHAR usbx_mem_pool[USBX_MEM_POOL_SIZE];
static UX_SLAVE_CLASS_CDC_ACM *g_cdc_acm;
static uint32_t g_last_send_ms;

static PCD_HandleTypeDef hpcd_usb1;


/* Full speed device framework. */
static UCHAR device_framework_full_speed[] = {
  /* Device descriptor */
  0x12, 0x01, 0x00, 0x02,
  0xEF, 0x02, 0x01,
  0x40,
  0x83, 0x04, 0x40, 0x57,
  0x00, 0x01,
  0x01, 0x02, 0x03,
  0x01,

  /* Configuration descriptor */
  0x09, 0x02, 0x4B, 0x00,
  0x02, 0x01, 0x00,
  0x80, 0x32,

  /* IAD */
  0x08, 0x0B, 0x00, 0x02, 0x02, 0x02, 0x01, 0x00,

  /* Communication Interface Descriptor */
  0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,

  /* Header Functional Descriptor */
  0x05, 0x24, 0x00, 0x10, 0x01,

  /* ACM Functional Descriptor */
  0x04, 0x24, 0x02, 0x02,

  /* Union Functional Descriptor */
  0x05, 0x24, 0x06, 0x00, 0x01,

  /* Call Management Functional Descriptor */
  0x05, 0x24, 0x01, 0x00, 0x01,

  /* Endpoint Descriptor (Interrupt IN) */
  0x07, 0x05, 0x83, 0x03, 0x10, 0x00, 0x10,

  /* Data Interface Descriptor */
  0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,

  /* Endpoint Descriptor (Bulk OUT) */
  0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,

  /* Endpoint Descriptor (Bulk IN) */
  0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00
};

static UCHAR string_framework[] = {
  /* Manufacturer string descriptor : Index 1 */
  0x09, 0x04, 0x01, 0x12,
  0x53, 0x54, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x65, 0x6C, 0x65, 0x63, 0x74,
  0x72, 0x6F, 0x6E, 0x69, 0x63, 0x73,

  /* Product string descriptor : Index 2 */
  0x09, 0x04, 0x02, 0x11,
  0x53, 0x54, 0x4D, 0x33, 0x32, 0x4E, 0x36, 0x35, 0x37, 0x30, 0x2D, 0x44,
  0x4B, 0x20, 0x43, 0x44, 0x43,

  /* Serial Number string descriptor : Index 3 */
  0x09, 0x04, 0x03, 0x04,
  0x30, 0x30, 0x30, 0x31
};

static UCHAR language_id_framework[] = {
  0x09, 0x04
};

static void USB_CDC_Activate(void *cdc_instance)
{
  g_cdc_acm = (UX_SLAVE_CLASS_CDC_ACM *)cdc_instance;
  BSP_LED_On(LED_RED);
}

static void USB_CDC_Deactivate(void *cdc_instance)
{
  (void)cdc_instance;
  g_cdc_acm = UX_NULL;
  BSP_LED_Off(LED_RED);
}

static void USB_CDC_ParamChange(void *cdc_instance)
{
  (void)cdc_instance;
}

static void USB_CDC_PCD_Init(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  (void)HAL_RCC_OscConfig(&RCC_OscInitStruct);

  memset(&hpcd_usb1, 0, sizeof(hpcd_usb1));
  hpcd_usb1.Instance = USB1_OTG_HS;
  hpcd_usb1.Init.dev_endpoints = 9U;
  hpcd_usb1.Init.speed = PCD_SPEED_FULL;
  hpcd_usb1.Init.dma_enable = DISABLE;
  hpcd_usb1.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
  hpcd_usb1.Init.Sof_enable = DISABLE;
  hpcd_usb1.Init.low_power_enable = DISABLE;
  hpcd_usb1.Init.lpm_enable = DISABLE;
  hpcd_usb1.Init.vbus_sensing_enable = DISABLE;
  hpcd_usb1.Init.use_dedicated_ep1 = DISABLE;
  hpcd_usb1.Init.use_external_vbus = DISABLE;
  if (HAL_PCD_Init(&hpcd_usb1) != HAL_OK)
  {
    return;
  }

  HAL_PCDEx_SetRxFiFo(&hpcd_usb1, 0x80);
  HAL_PCDEx_SetTxFiFo(&hpcd_usb1, 0, 0x10);
  HAL_PCDEx_SetTxFiFo(&hpcd_usb1, 1, 0x40);
  HAL_PCDEx_SetTxFiFo(&hpcd_usb1, 2, 0x40);
  HAL_PCDEx_SetTxFiFo(&hpcd_usb1, 3, 0x20);
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
  if (hpcd->Instance != USB1_OTG_HS)
  {
    return;
  }

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_SLEEP_ENABLE();

  HAL_PWREx_EnableVddUSBVMEN();
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_USB33RDY) == 0U) { }
  HAL_PWREx_EnableVddUSB();
  HAL_Delay(10);

  __HAL_RCC_USB1_OTG_HS_CLK_ENABLE();
  __HAL_RCC_USB1_OTG_HS_CLK_SLEEP_ENABLE();

  USB1_HS_PHYC->USBPHYC_CR &= ~(0x7U << 0x4U);
  USB1_HS_PHYC->USBPHYC_CR |= (0x2U << 0x4U);

  __HAL_RCC_USB1_OTG_HS_PHY_CLK_ENABLE();
  __HAL_RCC_USB1_OTG_HS_PHY_CLK_SLEEP_ENABLE();

  HAL_NVIC_SetPriority(USB1_OTG_HS_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(USB1_OTG_HS_IRQn);
}

void USB_CDC_Init(void)
{
  UX_SLAVE_CLASS_CDC_ACM_PARAMETER cdc_acm_param;
  UINT status;

  g_cdc_acm = UX_NULL;
  g_last_send_ms = 0U;
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_RED);
  BSP_LED_On(LED_GREEN);

  status = ux_system_initialize(usbx_mem_pool, USBX_MEM_POOL_SIZE, UX_NULL, 0);
  if (status != UX_SUCCESS)
  {
    return;
  }

  status = ux_device_stack_initialize(device_framework_full_speed, sizeof(device_framework_full_speed),
                                      device_framework_full_speed, sizeof(device_framework_full_speed),
                                      string_framework, sizeof(string_framework),
                                      language_id_framework, sizeof(language_id_framework), UX_NULL);
  if (status != UX_SUCCESS)
  {
    return;
  }

  memset(&cdc_acm_param, 0, sizeof(cdc_acm_param));
  cdc_acm_param.ux_slave_class_cdc_acm_instance_activate = USB_CDC_Activate;
  cdc_acm_param.ux_slave_class_cdc_acm_instance_deactivate = USB_CDC_Deactivate;
  cdc_acm_param.ux_slave_class_cdc_acm_parameter_change = USB_CDC_ParamChange;

  status = ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name,
                                          ux_device_class_cdc_acm_entry,
                                          1, 0, &cdc_acm_param);
  if (status != UX_SUCCESS)
  {
    return;
  }

  USB_CDC_PCD_Init();

  status = ux_dcd_stm32_initialize(0, (ULONG) &hpcd_usb1);
  if (status != UX_SUCCESS)
  {
    return;
  }

  HAL_PCD_Start(&hpcd_usb1);
}

void USB_CDC_Tasks(void)
{
  ux_device_stack_tasks_run();
  if (g_cdc_acm != UX_NULL)
  {
    _ux_device_class_cdc_acm_tasks_run(g_cdc_acm);
  }
}

void USB_CDC_SendDetections(const od_pp_out_t *p_postprocess, uint32_t inference_ms)
{
  uint32_t now;
  ULONG actual_length;
  char buffer[512];
  size_t used;
  int i;

  if (p_postprocess == NULL || g_cdc_acm == UX_NULL)
  {
    return;
  }
  if (p_postprocess->nb_detect > 0 && p_postprocess->pOutBuff == NULL)
  {
    return;
  }

  now = HAL_GetTick();
  if ((now - g_last_send_ms) < USB_CDC_TX_INTERVAL_MS)
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
    int written = snprintf(buffer + used, sizeof(buffer) - used,
                           "%s{\"c\":%ld,\"p\":%.3f,\"x\":%.3f,\"y\":%.3f,\"w\":%.3f,\"h\":%.3f}",
                           (i == 0) ? "" : ",",
                           (long)roi->class_index,
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

  ux_device_class_cdc_acm_write_run(g_cdc_acm, (UCHAR *)buffer, (ULONG)used, &actual_length);
}
