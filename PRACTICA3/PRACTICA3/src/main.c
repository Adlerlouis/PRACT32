#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h" 
#include  "esp_log.h"
#include "freertos/semphr.h"
//DEFIINIMOS NUESTROS TASK 
#define LEDG 35
#define LEDB 36
#define LEDR 37
#define STACK_SIZE 1024*2
#define RDELAY 1000
#define GDELAY 1000
#define BDELAY 1000
SemaphoreHandle_t Global_Key=0;

const char *tag ="Main";


int pot;//DECALRAMOS UN CONTADOR PARA PODER MOSTRAR EN PANTALLA

adc_channel_t  adc_pot  = ADC1_CHANNEL_3;   //DECLARAMOS EL PORT GPIO DE EL ADC

esp_err_t init_hw();//FUNCION ÁRA LLAMAR A LOS PERIFERICOS 
esp_err_t create_task(void);//CREAMOS LOS TASK

 void vTaskR( void * pvParameters);
 void vTaskG( void * pvParameters);
 void vTaskB( void * pvParameters);
 

void app_main(){
    init_hw();
    create_task();
    while(1){
       vTaskDelay(pdMS_TO_TICKS(100));
       printf("hello task \n");
     // printf("number of cores %i \n",portNUM_PROCESSORS); //IMPRIMIMOS EL NUMERO DE CORES QUE NECESITE
          
    }

}

esp_err_t init_hw(){
    gpio_reset_pin(LEDG);
    gpio_set_direction(LEDG,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MAS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)
    gpio_reset_pin(LEDB);
    gpio_set_direction(LEDB,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MENOS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)
    gpio_reset_pin(LEDR);
    gpio_set_direction(LEDR,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MENOS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)
    //DECALARACION ADC
    adc1_config_width(ADC_BITWIDTH_12); //ESCALA DEL ADC(ADC_ESCALA)
    adc1_config_channel_atten(adc_pot,ADC_ATTEN_DB_11); //AMPLITUD DE PASO DE ADC(VARIABLE_DE_MUESTREO,ESCALA_DE_PASO) 
    return ESP_OK;
}

esp_err_t create_task(void){
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;

    xTaskCreatePinnedToCore( vTaskR, "vTaskR", STACK_SIZE, &ucParameterToPass, 2, &xHandle,0); //DEFINIMOS LA TAREA1 EN EL CORE 0 
    xTaskCreatePinnedToCore( vTaskG, "vTaskG", STACK_SIZE, &ucParameterToPass, 1, &xHandle,0);//DEFINIMOS LA TAREA2 EN EL CORE 1
    xTaskCreatePinnedToCore( vTaskB, "vTaskB", STACK_SIZE, &ucParameterToPass, 1, &xHandle,0);//DEFINIMOS LA TAREA3 EN QUE QUIERA
   // vTaskStartScheduler();   


    return ESP_OK;
}

//DEFINIMOS QUE HACEN LAS TAREAS 
 void vTaskR (void * pvParameters )
  {
        TickType_t xStartTime = xTaskGetTickCount(); // Obtener el tiempo de inicio
    while(1)
    {   
            for (int i=0; i<10000000;i++ ){
              ESP_LOGI(tag,"LEDR CORE 0");
              gpio_set_level(LEDR,1);
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
             gpio_set_level(LEDR,0);

     
    }
    vTaskDelete(NULL);
  }


   void vTaskG (void * pvParameters )
  {
        TickType_t xStartTime = xTaskGetTickCount(); // Obtener el tiempo de inicio
    while(1)
    {   
      for (int i=0; i<10000000;i++ )
      {
        ESP_LOGW(tag,"LEDG CORE 1");
         gpio_set_level(LEDG,1);
      }
     vTaskDelay(pdMS_TO_TICKS(1000));
         gpio_set_level(LEDG,0);

     
    }
  }




   void vTaskB( void * pvParameters )
  {
        TickType_t xStartTime = xTaskGetTickCount(); // Obtener el tiempo de inicio
    while(1)
    {   
            for (int i=0; i<10000000;i++ ){
              ESP_LOGE(tag,"LEDB CORE 0");
              gpio_set_level(LEDB,1);
            }

                      gpio_set_level(LEDB,0);

         vTaskDelay(pdMS_TO_TICKS(1000));
         
     
    }
  }