/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "esp_http_server.h"

#include <string.h>
#include "driver/gpio.h"

// Definición de pines
#define RF_RECEIVER 13
#define RELAY_PIN_1 12
#define RELAY_PIN_2 14

// Definición de la tasa de baudios para la comunicación serial
#define SERIAL_BAUDRATE 115200

// Credenciales Wi-Fi
#define WIFI_SSID "L_Adler"
#define WIFI_PASS "Axel_reyes_123"

// Nombres de los dispositivos virtuale#define LAMP_1 "lamp one"
#define LAMP_1 "lampara 1"
#define LAMP_2 "lampara 2"


// Etiqueta para logs
static const char *TAG = "MAIN";

// Definición de la cola para RF
static xQueueHandle gpio_evt_queue = NULL;

// Prototipos de funciones
void wifi_init_sta(void);
static esp_err_t http_get_handler(httpd_req_t *req);
void app_main(void);

// Manejo de eventos de GPIO
static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Configuración Wi-Fi
void wifi_init_sta(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    // Esperar a que se conecte al AP
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

// Configuración del servidor HTTP
static esp_err_t http_get_handler(httpd_req_t *req) {
    const char resp[] = "Hello, world!";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = http_get_handler,
    .user_ctx = NULL
};

void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Iniciar el servidor HTTP
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Registrar URI
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &uri_get);
    } else {
        ESP_LOGI(TAG, "Error starting server!");
    }
}

// Función principal
void app_main(void) {
    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Configuración del GPIO
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = (1ULL << RF_RECEIVER);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << RELAY_PIN_1) | (1ULL << RELAY_PIN_2);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_set_level(RELAY_PIN_1, 1);
    gpio_set_level(RELAY_PIN_2, 1);

    // Crear cola para GPIO
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(RF_RECEIVER, gpio_isr_handler, (void *)RF_RECEIVER);

    // Iniciar Wi-Fi
    wifi_init_sta();

    // Iniciar servidor web
    start_webserver();

    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
            // Aquí se manejarían los eventos de RF
            if (io_num == RF_RECEIVER) {
                // Simulación de recepción de datos RF
                // Cambiar el estado del relé en función del valor recibido
                // Esta parte debe ser implementada con una librería RF adecuada
                if (gpio_get_level(RELAY_PIN_1)) {
                    gpio_set_level(RELAY_PIN_1, 0);
                } else {
                    gpio_set_level(RELAY_PIN_1, 1);
                }
                if (gpio_get_level(RELAY_PIN_2)) {
                    gpio_set_level(RELAY_PIN_2, 0);
                } else {
                    gpio_set_level(RELAY_PIN_2, 1);
                }
            }
        }
    }
}
