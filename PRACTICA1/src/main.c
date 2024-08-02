#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h" 
//------------------- SALIDAS  ------------------------------------
#define LED                 GPIO_NUM_2
 
#define LED1                GPIO_NUM_9
#define LED2                GPIO_NUM_10
#define GPIO_OUTPUT_LED_MASK    (1ULL<<LED |1ULL<<LED1 | 1ULL<<LED2 )
 
//------------------- ENTRADAS INTs --------------------------------
#define GPIO_INPUT_4        GPIO_NUM_4
#define GPIO_INPUT_5        GPIO_NUM_5
#define GPIO_INPUT_INT_MASK  ((1ULL<<GPIO_INPUT_4) | (1ULL<<GPIO_INPUT_5))
#define ESP_INTR_FLAG_DEFAULT 0
 
void funcion1(int variable);
int velocidad=500;
 
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    if(gpio_num==GPIO_INPUT_4)
    {
        gpio_set_level(LED, 1);
        if(velocidad>10) velocidad-=10;
    }
    if(gpio_num==GPIO_INPUT_5)
    {
        gpio_set_level(LED, 0);
        if(velocidad<1000) velocidad+=10;
    }  
}
 
void app_main(void)
{
    printf("inicializando\n");
 
    //Configurar pines de salida
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;      //disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;            //set as output mode
    io_conf.pin_bit_mask = GPIO_OUTPUT_LED_MASK;//bit mask of the pins that you want
    io_conf.pull_down_en = 0;                   //disable pull-down mode
    io_conf.pull_up_en = 0;                     //disable pull-up mode
    gpio_config(&io_conf);                      //configure GPIO with the given settings
 
    //Configurar interrupciones en falling edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;      //falling edge
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_INT_MASK;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
 
    //change gpio intrrupt type for one pin
    //gpio_set_intr_type(GPIO_INPUT_4, GPIO_INTR_ANYEDGE);
 
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_4, gpio_isr_handler, (void*) GPIO_INPUT_4);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_5, gpio_isr_handler, (void*) GPIO_INPUT_5);
 
    //remove isr handler for gpio number.
    //gpio_isr_handler_remove(GPIO_INPUT_4);
    //hook isr handler for specific gpio pin again
    //gpio_isr_handler_add(GPIO_INPUT_4, gpio_isr_handler, (void*) GPIO_INPUT_34);
     while(1) { 
        printf("LED1, velocidad=%d\n",velocidad);
        funcion1(1);
        vTaskDelay(velocidad / portTICK_PERIOD_MS);
     
        printf("LED2, velocidad=%d\n",velocidad);
        funcion1(2);
        vTaskDelay(velocidad / portTICK_PERIOD_MS);  
    }
}
 
void funcion1(int variable)
{
    //Apagar todo
    gpio_set_level(LED1, 0);
    gpio_set_level(LED2, 0);
 
    //Prender un led
    switch(variable)
    {
        case 1: gpio_set_level(LED1, 1); break;    //variable=1
        case 2: gpio_set_level(LED2, 1); break;    //variable=2
        default: break;
    }
}

 