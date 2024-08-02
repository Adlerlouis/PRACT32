#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h" 
#include  "esp_log.h"
#include "freertos/semphr.h"
//DEFIINIMOS NUESTROS TASK 
#define LEDR 35
//DEFINIMO LOS PINES PARA MOTOR A PASOS
#define LEDP 37
#define LEDP2 38
#define LEDP3 39
#define LEDP4 40

#define STACK_SIZE 1024*2
#define RDELAY 2000
#define BDELAY 2000

SemaphoreHandle_t  Global_Key=0;//semaforo global
SemaphoreHandle_t  Motor_Ready=0; //semaforo para tarea de motor a pasos

const char *tag ="Main";


int pot;//DECALRAMOS UN CONTADOR PARA PODER MOSTRAR EN PANTALLA

adc_channel_t  adc_pot  = ADC1_CHANNEL_0;   //DECLARAMOS EL PORT GPIO DE EL ADC

esp_err_t init_hw();//FUNCION ÁRA LLAMAR A LOS PERIFERICOS 
esp_err_t create_task(void);//CREAMOS LOS TASK
void motorpasos();

 void vTaskR( void * pvParameters);
 void vTaskMOTOR( void * pvParameters);
 void vTaskADC( void * pvParameters);
 

void app_main(){
    Global_Key =xSemaphoreCreateBinary();//inizial
    Motor_Ready = xSemaphoreCreateBinary();

    init_hw();
    create_task();

}

esp_err_t init_hw(){
    gpio_reset_pin(LEDR);
    gpio_set_direction(LEDR,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MAS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)
    //DECLARACION DE LOS PINES DE EL MOTOR A PASOS
     gpio_reset_pin(LEDP);
    gpio_set_direction(LEDP,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MENOS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)
    gpio_reset_pin(LEDP2);
    gpio_set_direction(LEDP2,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MENOS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)
    gpio_reset_pin(LEDP3);
    gpio_set_direction(LEDP3,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MENOS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)
    gpio_reset_pin(LEDP4);
    gpio_set_direction(LEDP4,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MENOS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)
    //DECALARACION ADC
    adc1_config_width(ADC_BITWIDTH_12); //ESCALA DEL ADC(ADC_ESCALA)
    adc1_config_channel_atten(adc_pot,ADC_ATTEN_DB_11); //AMPLITUD DE PASO DE ADC(VARIABLE_DE_MUESTREO,ESCALA_DE_PASO) 
    return ESP_OK;
}

esp_err_t create_task(void){
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;

    xTaskCreatePinnedToCore( vTaskR, "vTaskR", STACK_SIZE, &ucParameterToPass, 1, &xHandle,0);//DEFINIMOS LA TAREA2 EN EL CORE 1
    xTaskCreatePinnedToCore( vTaskADC, "vTaskADC", STACK_SIZE, &ucParameterToPass, 1, &xHandle,1);//DEFINIMOS LA TAREA3 EN QUE QUIERA
    xTaskCreatePinnedToCore( vTaskMOTOR, "vTaskMOTOR", STACK_SIZE, &ucParameterToPass, 1, &xHandle,1);//DEFINIMOS LA TAREA3 EN QUE QUIERA

   // vTaskStartScheduler();   


    return ESP_OK;
}

//DEFINIMOS QUE HACEN LAS TAREAS 
//TAREA DE LED MIENTRAS ESTE   
 void vTaskR (void * pvParameters )
  {
    while(1)
    {   
        
            for (int i=0; i<3;i++ ){
              vTaskDelay(pdMS_TO_TICKS(400));
              gpio_set_level(LEDR,1);
              vTaskDelay(pdMS_TO_TICKS(400));
              gpio_set_level(LEDR,0);

            }

            ESP_LOGI(tag,"task R DIO  LA LLAVE ");
            xSemaphoreGive(Global_Key);
            vTaskDelay(pdMS_TO_TICKS(RDELAY));

     
    }
  }




//generamos un ciclo del semaforo de 5 ciclos de trabajo ADC
   void vTaskADC( void * pvParameters )
  {
    while(1)
    {   
        int adc_value = 0;
         if(xSemaphoreTake(Global_Key,portMAX_DELAY)){
            ESP_LOGE(tag,"TASK ADC ESTA TRABAJANDO ");
            for (int i=0; i<5;i++ ){
            adc_value = adc1_get_raw(adc_pot); // Lectura del ADC 
              ESP_LOGI(tag, "Valor ADC: %d", adc_value); //MUESTRA EL VALOR DEL ADC 
            }
            ESP_LOGE(tag,"TASK ADC ESTA DURMIENDO ");

         }
            
            }
         vTaskDelay(pdMS_TO_TICKS(BDELAY));
         
     
    }
    //generamos un SEMAFORO DE 5 CICLOS DE TRABAJO DE MOTOR A PASOS
    void vTaskMOTOR( void * pvParameters )
  {
    while(1)
    {   

         if(xSemaphoreTake(Global_Key,portMAX_DELAY)){
            ESP_LOGE(tag,"TASK PASOS ESTA TRABAJANDO ");
                        for (int i=0; i<5;i++ ){

            motorpasos();
                        }
            ESP_LOGE(tag,"TASK PASOS ESTA DURMIENDO ");
            xSemaphoreGive(Motor_Ready); // Semaforo para poder acceder al motor a pasos 
         }
           vTaskDelay(pdMS_TO_TICKS(1000)); // Espera un segundo antes de verificar nuevamente

            
            }
    }
//Funcion para ciclo del motor a pasos 
    void motorpasos(){
      gpio_set_level(LEDP,1);
      gpio_set_level(LEDP2,0);
      gpio_set_level(LEDP3,0);
      gpio_set_level(LEDP4,0);
      vTaskDelay(pdMS_TO_TICKS(25));
      gpio_set_level(LEDP,0);
      gpio_set_level(LEDP2,1);
      gpio_set_level(LEDP3,0);
      gpio_set_level(LEDP4,0);
      vTaskDelay(pdMS_TO_TICKS(25));
       gpio_set_level(LEDP,0);
      gpio_set_level(LEDP2,0);
      gpio_set_level(LEDP3,1);
      gpio_set_level(LEDP4,0);
      vTaskDelay(pdMS_TO_TICKS(25));
        gpio_set_level(LEDP,0);
      gpio_set_level(LEDP2,0);
      gpio_set_level(LEDP3,0);
      gpio_set_level(LEDP4,1);
      vTaskDelay(pdMS_TO_TICKS(25));
    }