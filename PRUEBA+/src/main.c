#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h" 
int pot;//DECALRAMOS UN CONTADOR PARA PODER MOSTRAR EN PANTALLA

adc_channel_t  adc_pot  = ADC1_CHANNEL_3;   //DECLARAMOS EL PORT GPIO DE EL ADC

void init_hw(void);//FUNCION ÁRA LLAMAR A LOS PERIFERICOS 

void app_main(){
    init_hw();
    while(1){
     pot=adc1_get_raw(adc_pot);//Manda un echo al puerto para ver si valores 
    printf("ADC de un potenciometro  : %d\n",pot);//muestreo del ADC
				    //
    if (pot>=500){   //si este valor del ADC SUPERA EL PIN6 PRENDE Y EL PIN7 SE APAGA
      gpio_set_level(GPIO_NUM_6,1);
      gpio_set_level(GPIO_NUM_7,0);
    }
    else if(pot<=220){//si este valor del ADC NO LO SUPERA EL PIN6 SE APAGA
      gpio_set_level(GPIO_NUM_6,0);
      gpio_set_level(GPIO_NUM_7,1);
    }
    vTaskDelay(300/ portTICK_PERIOD_MS);//PERIDODO DE MUESTREO  
    }
}

void init_hw(void){
    gpio_set_direction(GPIO_NUM_6,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MAS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)
    gpio_set_direction(GPIO_NUM_7,GPIO_MODE_OUTPUT);//DECLARO MI GPIO CUANDO HAY MENOS LUZ(NUMERO_DE_PIN,MODO_DE_SALIDA)
    adc1_config_width(ADC_BITWIDTH_12); //ESCALA DEL ADC(ADC_ESCALA)
}
