#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"

#define MOTOR_PIN 45 // Pin al que está conectado el motor
#define PWM_CHANNEL 1// Canal PWM utilizado
#define PWM_FREQ 2500 // Frecuencia PWM en Hz
#define PWM_RESOLUTION LEDC_TIMER_10_BIT // Resolución PWM (10 bits)
#define LEDC_MODE   LEDC_LOW_SPEED_MODE

esp_err_t init_hw();
void PWM();

void app_main(){
    // Inicializar hardware
    init_hw();
    // Llamar a la función PWM
    PWM();
}

esp_err_t init_hw(){
    // Configurar el pin del motor como salida
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

void PWM(){
    // Establecer el duty cycle al 35% (aproximadamente 35 ms ON)
    ledc_set_duty(LEDC_MODE, PWM_CHANNEL, (1023 * 35 / 100)); // Escala de 0 a 1023
    ledc_update_duty(LEDC_MODE, PWM_CHANNEL);

    // Esperar un tiempo para mantener el PWM activo
    vTaskDelay(pdMS_TO_TICKS(35));

    // Apagar el PWM (establecer el duty cycle al 0)
    ledc_set_duty(LEDC_MODE, PWM_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, PWM_CHANNEL);
}

