

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/adc.h" 

#define LEDG 35
#define LEDR 36
#define STACK_SIZE 2048
#define RDELAY 1000
#define GDELAY 1000
adc_channel_t  adc_pot  = ADC1_CHANNEL_0;
QueueHandle_t GlobalQueue;

const char *tag = "Main";

void vTaskR(void *pvParameters);
void vTaskG(void *pvParameters);
void vTaskADC(void *pvParameters); // Cambiada vTaskB a vTaskADC

void app_main() {
    gpio_reset_pin(LEDG);
    gpio_set_direction(LEDG, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LEDR);
    gpio_set_direction(LEDR, GPIO_MODE_OUTPUT);
    adc1_config_width(ADC_BITWIDTH_12);
    adc1_config_channel_atten(adc_pot, ADC_ATTEN_DB_11);
    GlobalQueue = xQueueCreate(20, sizeof(int));
    if (GlobalQueue == NULL) {
        ESP_LOGE(tag, "Error creating queue");
    }

    xTaskCreatePinnedToCore(vTaskR, "vTaskR", STACK_SIZE, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(vTaskG, "vTaskG", STACK_SIZE, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(vTaskADC, "vTaskADC", STACK_SIZE, NULL, 1, NULL, 1); // Cambiado n  ombre de la tarea
}

void vTaskR(void *pvParameters) {
    int data_to_send = 0;
    while (1) {
        for (size_t i = 0; i < 8; i++) {
            vTaskDelay(pdMS_TO_TICKS(RDELAY / 2));
            gpio_set_level(LEDR, 1);
            if (xQueueSend(GlobalQueue, &data_to_send, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGI(tag, "Error sending %i to queue", i);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(RDELAY / 2));
        gpio_set_level(LEDR, 0);
    }
}

void vTaskG(void *pvParameters) {
    int received_data = 0;
    while (1) {
        if (xQueueReceive(GlobalQueue, &received_data, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGW(tag, "Error receiving value from queue");
        } else {
            vTaskDelay(pdMS_TO_TICKS(GDELAY / 2));
            gpio_set_level(LEDG, 1);
            ESP_LOGW(tag, "Received value from queue: %i", received_data);
            vTaskDelay(pdMS_TO_TICKS(GDELAY / 2));
            gpio_set_level(LEDG, 0);
        }
    }
}


void vTaskADC(void *pvParameters) {
    int adc_value = 0;
    while (1) {
        adc_value = adc1_get_raw(adc_pot); // Leer valor del ADC real
        if (xQueueSend(GlobalQueue, &adc_value, pdMS_TO_TICKS(1000)) != pdPASS) {
            ESP_LOGE(tag, "Error sending ADC value to queue");
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // Esperar 0.5 segundo antes de la próxima lectura
    }
}