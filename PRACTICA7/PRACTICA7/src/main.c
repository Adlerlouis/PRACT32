#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include  "esp_log.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "freertos/timers.h"
//DEFIINIMOS NUESTROS TASK 
#define LEDR 35
//DEFINIMO LOS PINES PARA MOTOR A PASOS
#define LEDP 37
#define LEDP2 38
#define LEDP3 39
#define LEDP4 40
//definimos la direccion de el motor 
#define R 1

uint8_t led_level =0;
//definimos EL pin para poder definir el pwm  y caracteristicas del PWM
#define MOTOR_PIN 45 // Pin al que está conectado el motor
#define PWM_CHANNEL 0 // Canal PWM utilizado
#define PWM_FREQ 2500 // Frecuencia PWM en Hz
#define PWM_RESOLUTION LEDC_TIMER_10_BIT // Resolución PWM (8 bits)
#define LEDC_MODE   LEDC_LOW_SPEED_MODE
#define STACK_SIZE 1024*5
#define BDELAY 2000
int interval=100;
#define L 2
int timer1= 1;
TimerHandle_t xTimers;
SemaphoreHandle_t  Global_Key=0;//semaforo global
SemaphoreHandle_t  Motor_Ready=0; //semaforo para tarea de motor a pasos

const char *tag ="Main";


int pot;//DECALRAMOS UN CONTADOR PARA PODER MOSTRAR EN PANTALLA


esp_err_t init_hw();//FUNCION ÁRA LLAMAR A LOS PERIFERICOS 
esp_err_t create_task(void);//CREAMOS LOS TASK
esp_err_t shared_resource(int LED);
esp_err_t timer(SemaphoreHandle_t semaphore);
esp_err_t level(void);


 void motorpasos();
 void PWM();
 void vTaskR( void * pvParameters);
 void vTaskMOTOR( void * pvParameters);
 void vTaskPWM( void * pvParameters);
 
 void vTimerCallback(TimerHandle_t pxTimer){
    ESP_LOGW(tag,"El evento fue llamado por el timmer");
         BaseType_t xHigherPriorityTaskWoken = pdFALSE; 
    xSemaphoreGiveFromISR(Global_Key, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE) {
              gpio_set_level(L,1);

        portYIELD_FROM_ISR();
    }
   
}

void app_main(){
    Global_Key =xSemaphoreCreateBinary();
    Motor_Ready = xSemaphoreCreateBinary();
    init_hw();
    create_task();
    timer(Global_Key);


  

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
    //direccion del motor PWM
     gpio_reset_pin(R);
    gpio_set_direction(R,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MENOS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)
    
    gpio_reset_pin(L);
    gpio_set_direction(R,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MENOS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)

    //DECALARACION PWM
   
      // Configurar el pin del motor como salida
    esp_rom_gpio_pad_select_gpio(MOTOR_PIN);
    gpio_set_direction(MOTOR_PIN, GPIO_MODE_OUTPUT);

    // Configurar la configuración de PWM
    ledc_timer_config_t timer_conf = {
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQ,
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&timer_conf);

    // Configurar el canal PWM
    ledc_channel_config_t pwm_conf = {
        .gpio_num = MOTOR_PIN,
        .speed_mode = LEDC_MODE,
        .channel = PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&pwm_conf);
    return ESP_OK;
}

esp_err_t create_task(void){
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;

    xTaskCreatePinnedToCore( vTaskR, "vTaskR", STACK_SIZE, &ucParameterToPass, 1, &xHandle,0);//DEFINIMOS LA TAREA2 EN EL CORE 1
    xTaskCreatePinnedToCore( vTaskPWM, "vTaskPWM", STACK_SIZE, &ucParameterToPass, 1, &xHandle,1);//DEFINIMOS LA TAREA3 EN QUE QUIERA
    xTaskCreatePinnedToCore( vTaskMOTOR, "vTaskMOTOR", STACK_SIZE, &ucParameterToPass, 1, &xHandle,1);//DEFINIMOS LA TAREA3 EN QUE QUIERA

   // vTaskStartScheduler();   


    return ESP_OK;
}
esp_err_t timer(SemaphoreHandle_t semaphore){
            ESP_LOGI(tag,"Timer init configuracion");
            xTimers = xTimerCreate("Timer",(pdMS_TO_TICKS(interval)),pdTRUE,( void *)timer1,vTimerCallback);
      if( xTimers == NULL )
         {
                        ESP_LOGI(tag,"Timer no fue creado");

             // The timer was not created.
         }
         else
         {
             
           if( xTimerStart( xTimers, 0 ) != pdPASS )
           {
                        ESP_LOGI(tag,"Timer no esta en estado activo");

                // The timer could not be set into the Active state.
             }
         }
         return ESP_OK;
    }


esp_err_t level(void){
     led_level=!led_level;
     gpio_set_level(R,led_level);
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
            vTaskDelay(pdMS_TO_TICKS(1000));

     
    }
  }




//generamos un ciclo del semaforo de 5 ciclos de trabajo ADC
   void vTaskPWM( void * pvParameters )
  {
    while(1){
            if(xSemaphoreTake(Global_Key,portMAX_DELAY)==pdTRUE){
                         ESP_LOGE(tag,"Task PWM  TOMADO EL RECURSO ");
                         PWM();
              }         
                       vTaskDelay(pdMS_TO_TICKS(BDELAY));

                  }
  }
         

            
         
     
    
    //generamos un SEMAFORO DE 5 CICLOS DE TRABAJO DE MOTOR A PASOS
    void vTaskMOTOR( void * pvParameters )
  {
    while(1)
    {   

         if(xSemaphoreTake(Global_Key,portMAX_DELAY)==pdTRUE){
            ESP_LOGE(tag,"TASK PASOS TOMANDO RECURSOS EN ESTA TAREA");
            for (int i=0; i<5;i++ ){
            motorpasos();
                      }
           xSemaphoreGive(Global_Key); // Semaforo para poder acceder al motor a pasos 

         
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


    void PWM(){
          gpio_set_level(R,1);
          ledc_set_duty(LEDC_MODE, PWM_CHANNEL, 0);
          ledc_update_duty(LEDC_MODE, PWM_CHANNEL);
          vTaskDelay(pdMS_TO_TICKS(20)); // Esperar un poco antes de aumentar la velocidad nuevamente
       for (int ciclo_de_trabajo = 0; ciclo_de_trabajo <= 100; ciclo_de_trabajo++) {
            // Cambiar la velocidad del motor
            ledc_set_duty(LEDC_MODE, PWM_CHANNEL, ciclo_de_trabajo * (1023 / 100)); // Escala de 0 a 1023
            ledc_update_duty(LEDC_MODE, PWM_CHANNEL);
            ESP_LOGI(tag,"Task PWM TOMADO EL RECURSO EN ESTA TAREA ");
            printf("ciclo motor a pasos %d\n",ciclo_de_trabajo);
            vTaskDelay(pdMS_TO_TICKS(20)); // Esperar un poco antes de cambiar la velocidad nuevamente
                  }
           gpio_set_level(R,0);

    }
    
