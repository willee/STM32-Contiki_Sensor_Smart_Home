/**

  ******************************************************************************
  */ 
 
#include "stm32f10x.h"
#include <string.h>
#include "bsp_gpio.h"
#include "bsp_usart1.h"
#include "bsp_usart2.h"
#include "wifi_config.h"
#include "wifi_function.h"
#include "bsp_SysTick.h"
#include "dma.h"
#include "iwdg.h"
#include "SPI.h"
#include "oled.h"
#include "dht11.h"
#include "delay.h"

#include "contiki-conf.h"
#include <stdint.h>
#include <stdio.h>
#include <debug-uart.h>
#include <process.h>
#include <procinit.h>
#include <etimer.h>
#include <autostart.h>
#include <clock.h>
#include "contiki_delay.h"


//#define __WIFI_MODULE_ON__
//#define __OLED_MODULE_ON__
#define __DHT11_MODULE_ON__


PROCESS(red_blink_process, "Red Blink");
PROCESS(green_blink_process, "Green Blink");
PROCESS(IWDG_Feed_process, "Timing to feed dog");
PROCESS(clock_test_process, "Test system delay");

PROCESS(wifi_send_test_process, "Wifi module send data test");
PROCESS(OLED_Show_Increment_process, "Show a increment num in OLED");
PROCESS(DHT11_Read_Data_process, "DHT11 read temperature and humidity test");

AUTOSTART_PROCESSES(&etimer_process,&IWDG_Feed_process);



void BSP_Config(void)
{
    /* 初始化 */
    delay_init();
    clock_init();
    LED_GPIO_Config();
    USART1_Config(115200);
    
#ifdef __WIFI_MODULE_ON__
    WiFi_Config(); //初始化WiFi模块使用的接口和外设
#endif
    
#ifdef __OLED_MODULE_ON__
    OLED_Init(); //初始化OLED模块使用的接口和外设
#endif         
    
#ifdef __DHT11_MODULE_ON__
    DHT11_Init(); //初始化OLED模块使用的接口和外设
#endif     
    
}


int main(void)
{	
    BSP_Config();    
    
#ifdef __OLED_MODULE_ON__
    {
        OLED_ShowString(0,0,"SPI OLED");
        OLED_ShowString(0,16,"Start OK!");
        OLED_Refresh_Gram();//更新显示
    }
#endif 

#ifdef __WIFI_MODULE_ON__    
    ESP8266_STA_TCP_Client();
#endif
    
    IWDG_Start(2);  //wifi模块透传之后开启看门狗
    
    process_init();
    autostart_start(autostart_processes);
    process_start(&red_blink_process,NULL);
    process_start(&green_blink_process,NULL);
    //process_start(&clock_test_process,NULL);
#ifdef __OLED_MODULE_ON__
    process_start(&OLED_Show_Increment_process,NULL);
#endif
    
#ifdef __DHT11_MODULE_ON__
    process_start(&DHT11_Read_Data_process,NULL);
#endif   
    
#ifdef __WIFI_MODULE_ON__     
    process_start(&wifi_send_test_process,NULL);
#endif

    while (1)
    {
        do
        {
        }while (process_run()>0);
    }
}


/*******************PROCESS************************/


PROCESS_THREAD(red_blink_process, ev, data)
{
    static struct etimer et;
    PROCESS_BEGIN();
    while(1)
    {
        Contiki_etimer_DelayMS(500);
        GPIO_SetBits(GPIOA, GPIO_Pin_8);
        Contiki_etimer_DelayMS(500);
        GPIO_ResetBits(GPIOA, GPIO_Pin_8);
    }
    PROCESS_END();
}

PROCESS_THREAD(green_blink_process, ev, data)
{
    static struct etimer et;
    PROCESS_BEGIN();
    while(1)
    {        
        Contiki_etimer_DelayMS(200);
        GPIO_SetBits(GPIOD, GPIO_Pin_2);
        Contiki_etimer_DelayMS(200);
        GPIO_ResetBits(GPIOD, GPIO_Pin_2);
    }
    PROCESS_END();
}

PROCESS_THREAD(wifi_send_test_process, ev, data)
{
    static struct etimer et;
    PROCESS_BEGIN();
    while(1)
    {
        
        Contiki_etimer_DelayMS(500);
        UART2_SendBuff = " asdfafasdf";
        UART2_DMA_Send_Data(UART2_SendBuff, 11);
        
        Contiki_etimer_DelayMS(500);        
        UART2_SendBuff = " 21398416hy";
        UART2_DMA_Send_Data(UART2_SendBuff, 11);
    }
    PROCESS_END();
}

PROCESS_THREAD(OLED_Show_Increment_process, ev, data)
{
    static struct etimer et;
    static int count;
    uint8_t temperature;
    uint8_t temperature0;	 
	uint8_t humidity;    
    uint8_t humidity0;
    PROCESS_BEGIN();
    while(1)
    {
        OLED_ShowNum(0,32,count++,5,16);
        
#ifdef __DHT11_MODULE_ON__
        DHT11_Read_Data(&temperature,&temperature0,&humidity,&humidity0);
        OLED_ShowNum(0,48,humidity,5,16);
#endif
        
        OLED_Refresh_Gram();//更新显示
        Contiki_etimer_DelayMS(500);
    }
    PROCESS_END();
}

PROCESS_THREAD(DHT11_Read_Data_process, ev, data)
{
    static struct etimer et;
    uint8_t temperature;
    uint8_t temperature0;	 
	uint8_t humidity;    
    uint8_t humidity0;
    PROCESS_BEGIN();
    while(1)
    {
        DHT11_Read_Data(&temperature,&temperature0,&humidity,&humidity0);
        printf("temperature: %.2f°C  humidity: %.2f \r\n",(float)temperature+(float)temperature0*0.01,(float)humidity+(float)humidity0*0.01);	
        Contiki_etimer_DelayMS(500);
    }
    PROCESS_END();
}

PROCESS_THREAD(clock_test_process, ev, data)
{
    static uint16_t i,start_count,end_count,diff;
  PROCESS_BEGIN();

  printf("Clock delay test, (10,000 x i) cycles:\n");
  i = 1;
  while(i < 16) {
    start_count = clock_time();                   // 记录开始timer
    Delay_NOP_ms(10 * i);                       // 软件延时
    end_count = clock_time();                     // 记录结束timer
    diff = end_count - start_count;               // 计算差值，单位为tick
    printf("Delayed %u \n%u ticks =~ %u ms\n", 10 * i, diff, diff * 10);
    i++;
  }

  printf("Done!\n");

  PROCESS_END();
}

PROCESS_THREAD(IWDG_Feed_process, ev, data)
{
    static struct etimer et;
    PROCESS_BEGIN();
    while(1)
    {
        IWDG_Feed();
        Contiki_etimer_DelayMS(1000);
    }
    PROCESS_END();
}

