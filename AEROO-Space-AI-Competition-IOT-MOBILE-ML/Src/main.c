/**
  ******************************************************************************
  * @file    Wifi/WiFi_HTTP_Server/src/main.c
  * @author  MCD Application Team
  * @brief   This file provides main program functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"


/* Private defines -----------------------------------------------------------*/
/* Update SSID and PASSWORD with own Access point settings */
#define SSID     "OPPO A74"
#define PASSWORD "h5x7bacs"
#define PORT           80

#define TERMINAL_USE


#define WIFI_WRITE_TIMEOUT 10000
#define WIFI_READ_TIMEOUT  10000
#define SOCKET                 0


#ifdef  TERMINAL_USE
#define LOG(a) printf a
#else
#define LOG(a)
#endif


/* Private typedef------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#if defined (TERMINAL_USE)
extern UART_HandleTypeDef hDiscoUart;
#endif /* TERMINAL_USE */

static  uint8_t http[1024];  /* Original size */
static  uint8_t  IP_Addr[4];
static  int     LedState = 0;

/* Private function prototypes -----------------------------------------------*/
#if defined (TERMINAL_USE)
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
#endif /* TERMINAL_USE */

static void SystemClock_Config(void);
static  WIFI_Status_t SendWebPage(uint8_t ledIsOn, uint8_t temperature, uint8_t humidity);
static  WIFI_Status_t SendJSONResponse(uint8_t temperature, uint8_t humidity);
static  int wifi_server(void);
static  int wifi_start(void);
static  int wifi_connect(void);
static  bool WebServerProcess(void);



/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure LED2 */
  BSP_LED_Init(LED2);

  /*Initialize Temperature and humidity sensor */
 // HAL_ADCEx_Calibration_Start(&AdcHandle, ADC_SINGLE_ENDED);
  //HAL_ADC_Start(&AdcHandle) ;

  /* WIFI Web Server demonstration */
#if defined (TERMINAL_USE)
  /* Initialize all configured peripherals */
  hDiscoUart.Instance = DISCOVERY_COM1;
  hDiscoUart.Init.BaudRate = 115200;
  hDiscoUart.Init.WordLength = UART_WORDLENGTH_8B;
  hDiscoUart.Init.StopBits = UART_STOPBITS_1;
  hDiscoUart.Init.Parity = UART_PARITY_NONE;
  hDiscoUart.Init.Mode = UART_MODE_TX_RX;
  hDiscoUart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hDiscoUart.Init.OverSampling = UART_OVERSAMPLING_16;
  hDiscoUart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hDiscoUart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;


  BSP_COM_Init(COM1, &hDiscoUart);
  BSP_TSENSOR_Init();
  BSP_HSENSOR_Init();

  printf("****** WIFI Web Server demonstration****** \n\n");

#endif /* TERMINAL_USE */

  wifi_server();
}

/**
  * @brief  Send HTML page
  * @param  None
  * @retval None
  */


static int wifi_start(void)
{
  uint8_t  MAC_Addr[6];

 /*Initialize and use WIFI module */
  if(WIFI_Init() ==  WIFI_STATUS_OK)
  {
    LOG(("ES-WIFI Initialized.\n"));
    if(WIFI_GetMAC_Address(MAC_Addr, sizeof(MAC_Addr)) == WIFI_STATUS_OK)
    {
      LOG(("> eS-WiFi module MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n",
               MAC_Addr[0],
               MAC_Addr[1],
               MAC_Addr[2],
               MAC_Addr[3],
               MAC_Addr[4],
               MAC_Addr[5]));
    }
    else
    {
      LOG(("> ERROR : CANNOT get MAC address\n"));
      return -1;
    }
  }
  else
  {
    return -1;
  }
  return 0;
}



int wifi_connect(void)
{

  wifi_start();

  LOG(("\nConnecting to %s , %s\n",SSID,PASSWORD));
  if( WIFI_Connect(SSID, PASSWORD, WIFI_ECN_WPA2_PSK) == WIFI_STATUS_OK)
  {
    if(WIFI_GetIP_Address(IP_Addr, sizeof(IP_Addr)) == WIFI_STATUS_OK)
    {
      LOG(("> es-wifi module connected: got IP Address : %d.%d.%d.%d\n",
               IP_Addr[0],
               IP_Addr[1],
               IP_Addr[2],
               IP_Addr[3]));
    }
    else
    {
		  LOG((" ERROR : es-wifi module CANNOT get IP address\n"));
      return -1;
    }
  }
  else
  {
		 LOG(("ERROR : es-wifi module NOT connected\n"));
     return -1;
  }
  return 0;
}

int wifi_server(void)
{
  bool StopServer = false;

  LOG(("\nRunning HTML Server test\n"));
  if (wifi_connect()!=0) return -1;


  if (WIFI_STATUS_OK!=WIFI_StartServer(SOCKET, WIFI_TCP_PROTOCOL, 1, "", PORT))
  {
    LOG(("ERROR: Cannot start server.\n"));
  }

  LOG(("Server is running and waiting for an HTTP  Client connection to %d.%d.%d.%d\n",IP_Addr[0],IP_Addr[1],IP_Addr[2],IP_Addr[3]));

  do
  {
    uint8_t RemoteIP[4];
    uint16_t RemotePort;


    while (WIFI_STATUS_OK != WIFI_WaitServerConnection(SOCKET,1000,RemoteIP,sizeof(RemoteIP), &RemotePort))
    {
        LOG(("Waiting connection to  %d.%d.%d.%d\n",IP_Addr[0],IP_Addr[1],IP_Addr[2],IP_Addr[3]));

    }

    LOG(("Client connected %d.%d.%d.%d:%d\n",RemoteIP[0],RemoteIP[1],RemoteIP[2],RemoteIP[3],RemotePort));

    StopServer=WebServerProcess();

    if(WIFI_CloseServerConnection(SOCKET) != WIFI_STATUS_OK)
    {
      LOG(("ERROR: failed to close current Server connection\n"));
      return -1;
    }
  }
  while(StopServer == false);

  if (WIFI_STATUS_OK!=WIFI_StopServer(SOCKET))
  {
    LOG(("ERROR: Cannot stop server.\n"));
  }

  LOG(("Server is stop\n"));
  return 0;
}


static bool WebServerProcess(void)
{
  uint8_t temp;
  uint8_t hum;
  uint16_t  respLen;
  static   uint8_t resp[1024];
  bool    stopserver=false;

  if (WIFI_STATUS_OK == WIFI_ReceiveData(SOCKET, resp, 1000, &respLen, WIFI_READ_TIMEOUT))
  {
   LOG(("get %d byte from server\n",respLen));

   if( respLen > 0)
   {
      if(strstr((char *)resp, "GET")) /* GET: put web page */
      {
        temp = (int) BSP_TSENSOR_ReadTemp();
        hum= (int) BSP_HSENSOR_ReadHumidity();

        /* Check if this is an API request for JSON */
        if(strstr((char *)resp, "GET /api/temperature"))
        {
          if(SendJSONResponse(temp, hum) != WIFI_STATUS_OK)
          {
            LOG(("> ERROR : Cannot send JSON response\n"));
          }
          else
          {
            LOG(("Send JSON after GET /api/temperature\n"));
          }
        }
        else
        {
          /* Normal web page request */
          if(SendWebPage(LedState, temp, hum) != WIFI_STATUS_OK)
          {
            LOG(("> ERROR : Cannot send web page\n"));
          }
          else
          {
            LOG(("Send page after  GET command\n"));
          }
        }
       }
       else if(strstr((char *)resp, "POST"))/* POST: received info */
       {
         LOG(("Post request\n"));

         if(strstr((char *)resp, "radio"))
         {
           if(strstr((char *)resp, "radio=0"))
           {
             LedState = 0;
             BSP_LED_Off(LED2);
           }
           else if(strstr((char *)resp, "radio=1"))
           {
             // Mode 1: >70% - Green (Healthy) - LED ON steady
             LedState = 1;
             BSP_LED_On(LED2);
           }
           else if(strstr((char *)resp, "radio=2"))
           {
             // Mode 2: 50-70% - Yellow (Warning) - LED ON (blink simulated on web)
             LedState = 2;
             BSP_LED_On(LED2);
           }
           else if(strstr((char *)resp, "radio=3"))
           {
             // Mode 3: <50% - Red (Critical) - LED ON (fast blink simulated on web)
             LedState = 3;
             BSP_LED_On(LED2);
           }
           temp = (int) BSP_TSENSOR_ReadTemp();
           hum= (int) BSP_HSENSOR_ReadHumidity();
         }
         if(strstr((char *)resp, "stop_server"))
         {
           if(strstr((char *)resp, "stop_server=0"))
           {
             stopserver = false;
           }
           else if(strstr((char *)resp, "stop_server=1"))
           {
             stopserver = true;
           }
         }
         temp = (int) BSP_TSENSOR_ReadTemp();
         hum= (int) BSP_HSENSOR_ReadHumidity();
         if(SendWebPage(LedState, temp, hum) != WIFI_STATUS_OK)
         {
           LOG(("> ERROR : Cannot send web page\n"));
         }
         else
         {
           LOG(("Send Page after POST command\n"));
         }
       }
     }
  }
  else
  {
    LOG(("Client close connection\n"));
  }
  return stopserver;

 }

/**
  * @brief  Send HTML page
  * @param  None
  * @retval None
  */
static WIFI_Status_t SendWebPage(uint8_t ledIsOn, uint8_t temperature, uint8_t humidity)
{
  uint8_t temp[16];
  uint8_t hum[16];
  uint16_t SentDataLength;
  WIFI_Status_t ret;

  strcpy((char *)http,
         "HTTP/1.0 200 OK\r\n"
         "Content-Type: text/html\r\n"
         "Access-Control-Allow-Origin: *\r\n"
         "Pragma: no-cache\r\n\r\n");

  strcat((char *)http, "<html><head><title>Battery Monitor</title>");
  strcat((char *)http, "<style>body{font-family:Arial;margin:20px}");
  strcat((char *)http, ".row{margin:10px 0}b{color:#333}</style></head><body>");
  strcat((char *)http, "<h2>Battery Health Monitor</h2>");
  strcat((char *)http, "<div class=\"row\"><b>Temp:</b> <span id=\"t\">");
  sprintf((char *)temp, "%d", temperature);
  strcat((char *)http, (char *)temp);
  strcat((char *)http, "</span>C | <b>Hum:</b> <span id=\"h\">");
  sprintf((char *)hum, "%d", humidity);
  strcat((char *)http, (char *)hum);
  strcat((char *)http, "</span>%</div>");
  strcat((char *)http, "<div class=\"row\"><b>Voltage:</b> <span id=\"v\">-</span>V | <b>Current:</b> <span id=\"c\">-</span>A</div>");
  strcat((char *)http, "<div id=\"p\">Loading...</div>");
  strcat((char *)http, "<div class=\"row\"><b>LED:</b> <span id=\"led\" style=\"padding:5px 10px;border-radius:5px\">-</span></div>");

  strcat((char *)http, "<script>let iv=null,lastMode=0;");
  strcat((char *)http, "function setL(m){if(m!=lastMode){fetch('',{method:'POST',body:'radio='+m}).catch(e=>0);lastMode=m;}}");
  strcat((char *)http, "function upd(){fetch('/api/temperature').then(r=>r.json()).then(d=>{");
  strcat((char *)http, "document.getElementById('t').innerHTML=d.temperature;");
  strcat((char *)http, "document.getElementById('h').innerHTML=d.humidity;");
  strcat((char *)http, "return fetch('http://localhost:5000/predict?temperature='+d.temperature);");
  strcat((char *)http, "}).then(r=>r.json()).then(d=>{");
  strcat((char *)http, "let h=d.prediction.battery_health_percent;");
  strcat((char *)http, "document.getElementById('v').innerHTML=d.measurements.voltage.toFixed(2);");
  strcat((char *)http, "document.getElementById('c').innerHTML=d.measurements.current.toFixed(2);");
  strcat((char *)http, "document.getElementById('p').innerHTML='<b>Battery: '+h.toFixed(1)+'%</b> ('+d.prediction.health_status+')';");
  strcat((char *)http, "let l=document.getElementById('led');");
  strcat((char *)http, "if(iv)clearInterval(iv);");
  strcat((char *)http, "if(h>70){l.innerHTML='GREEN (Healthy)';l.style.background='#4CAF50';l.style.color='#fff';l.style.opacity=1;setL(1);}");
  strcat((char *)http, "else if(h>50){l.innerHTML='YELLOW (Warning)';l.style.background='#FFC107';l.style.color='#333';setL(2);");
  strcat((char *)http, "let s=0;iv=setInterval(()=>{s=!s;l.style.opacity=s?1:0.3},800);}");
  strcat((char *)http, "else{l.innerHTML='RED (CRITICAL)';l.style.background='#F44336';l.style.color='#fff';setL(3);");
  strcat((char *)http, "let s=0;iv=setInterval(()=>{s=!s;l.style.opacity=s?1:0.3},500);}");
  strcat((char *)http, "}).catch(e=>document.getElementById('p').innerHTML='Backend Error')}");
  strcat((char *)http, "upd();setInterval(upd,10000);</script></body></html>");

  ret = WIFI_SendData(0, (uint8_t *)http, strlen((char *)http),
                      &SentDataLength, WIFI_WRITE_TIMEOUT);

  if ((ret == WIFI_STATUS_OK) && (SentDataLength != strlen((char *)http)))
  {
    ret = WIFI_STATUS_ERROR;
  }

  return ret;
}

/**
  * @brief  Send JSON response for API endpoint
  * @param  temperature: Temperature value
  * @param  humidity: Humidity value
  * @retval WIFI_Status_t
  */
static WIFI_Status_t SendJSONResponse(uint8_t temperature, uint8_t humidity)
{
  uint16_t SentDataLength;
  WIFI_Status_t ret;

  /* Clear buffer */
  memset(http, 0, sizeof(http));

  /* Build JSON response */
  strcpy((char *)http,
         "HTTP/1.0 200 OK\r\n"
         "Content-Type: application/json\r\n"
         "Access-Control-Allow-Origin: *\r\n"
         "Pragma: no-cache\r\n\r\n");

  strcat((char *)http, "{");
  strcat((char *)http, "\"temperature\":");
  sprintf((char *)&http[strlen((char *)http)], "%d", temperature);
  strcat((char *)http, ",\"humidity\":");
  sprintf((char *)&http[strlen((char *)http)], "%d", humidity);
  strcat((char *)http, "}");

  /* Send JSON data */
  ret = WIFI_SendData(0, (uint8_t *)http, strlen((char *)http),
                      &SentDataLength, WIFI_WRITE_TIMEOUT);

  if ((ret == WIFI_STATUS_OK) && (SentDataLength != strlen((char *)http)))
  {
    ret = WIFI_STATUS_ERROR;
  }

  return ret;
}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (MSI)
  *            SYSCLK(Hz)                     = 80000000
  *            HCLK(Hz)                       = 80000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            MSI Frequency(Hz)              = 4000000
  *            PLL_M                          = 1
  *            PLL_N                          = 40
  *            PLL_R                          = 2
  *            PLL_P                          = 7
  *            PLL_Q                          = 4
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* MSI is enabled after System reset, activate PLL with MSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLP = 7;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    /* Initialization Error */
    while(1);
  }
}

#if defined (TERMINAL_USE)
/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
  HAL_UART_Transmit(&hDiscoUart, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}
#endif /* TERMINAL_USE */

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif


/**
  * @brief  EXTI line detection callback.
  * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI line.
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
    case (GPIO_PIN_1):
    {
      SPI_WIFI_ISR();
      break;
    }
    default:
    {
      break;
    }
  }
}

/**
  * @brief  SPI3 line detection callback.
  * @param  None
  * @retval None
  */
void SPI3_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi);
}


