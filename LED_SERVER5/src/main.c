#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "protocol_examples_common.h"
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include <string.h>
#include "protocol.h"
#include <esp_http_server.h>
#include "driver/gpio.h"
#include "rom/gpio.h"               //adicionada

int Pin1 =0; //banderas ON/OFF
int Pin2 =0;

#define Pin1IO 2
#define Pin2IO 25
//definicion de pines para motor a pasos
#define LEDP 37
#define LEDP2 38
#define LEDP3 39
#define LEDP4 40
void motorpasos();
void motorpasos2();
void motor();
// Assign output variables to GPIO pins
//const int output26 = 26;
//const int output27 = 27;

// Display the HTML web page, Boton2=0, Boton1=0
char HTMLWebPage00[]="<!DOCTYPE html><html>"
            "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
            "<link rel=\"icon\" href=\"data:,\">"
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"
            ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;"
            "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
            ".button2 {background-color: #555555;}</style></head>"
            "<body><h1>ESP32 Web Server</h1>"// Web Page Heading
            "<p>Pin 1 - Apagado</p>"
            "<p><a href=\"/Pin1/on\"><button class=\"button button2\">OFF</button></a></p>"
            "<p>Pin 2 - Apagado</p>"
            "<p><a href=\"/Pin2/on\"><button class=\"button button2\">OFF</button></a></p>"
            "</body></html>";

// HTML Boton2=0, Boton1=1
char HTMLWebPage01[]="<!DOCTYPE html><html>"
            "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
            "<link rel=\"icon\" href=\"data:,\">"
            "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"
            ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;"
            "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
            ".button2 {background-color: #555555;}</style></head>"
            "<body><h1>ESP32 Web Server</h1>"
            "<p>Pin 1 - ABIERTO</p>"// Display current state, and ON/OFF buttons for Pin 1 (GPIO 26)
            "<p><a href=\"/Pin1/off\"><button class=\"button\">ON</button></a></p>"
            "<p>Pin 2 - Apagado</p>"// Display current state, and ON/OFF buttons for Pin 2 (GPIO 27)
            "<p><a href=\"/Pin2/on\"><button class=\"button button2\">OFF</button></a></p>"
            "</body></html>";
// HTML Boton2=1, Boton1=0
char HTMLWebPage10[]="<!DOCTYPE html><html>"
            "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
            "<link rel=\"icon\" href=\"data:,\">"
            "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"
            ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;"
            "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
            ".button2 {background-color: #555555;}</style></head>"
            "<body><h1>ESP32 Web Server</h1>"
            "<p>Pin 1 - Apagado</p>"// Display current state, and ON/OFF buttons for Pin 1 (GPIO 26)
            "<p><a href=\"/Pin1/on\"><button class=\"button button2\">OFF</button></a></p>"
            "<p>Pin 2 - CERRADO</p>"// Display current state, and ON/OFF buttons for Pin 2 (GPIO 27)
            "<p><a href=\"/Pin2/off\"><button class=\"button\">ON</button></a></p>"
            "</body></html>";
// HTML Boton2=1, Boton1=1
char HTMLWebPage11[]="<!DOCTYPE html><html>"
            "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
            "<link rel=\"icon\" href=\"data:,\">"
            "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}"
            ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;"
            "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}"
            ".button2 {background-color: #555555;}</style></head>"
            "<body><h1>ESP32 Web Server</h1>"
            "<p>Pin 1 - State ON</p>"// Display current state, and ON/OFF buttons for Pin 1 (GPIO 26)
            "<p><a href=\"/Pin1/off\"><button class=\"button\">ON</button></a></p>"
            "<p>Pin 2 - State ON</p>"// Display current state, and ON/OFF buttons for Pin 2 (GPIO 27)
             "<p><a href=\"/Pin2/off\"><button class=\"button\">ON</button></a></p>"
            "</body></html>";


static const char *TAG = "Webserver 2LED";

/********************************************** SERVER *****************************************/
/* An HTTP GET handler */
static esp_err_t home_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

static esp_err_t Pin1on_get_handler(httpd_req_t *req)
{
    const char* resp_str;
    if(Pin2==0) {resp_str = (const char*) HTMLWebPage01;}
    else {resp_str = (const char*) HTMLWebPage11;}
    httpd_resp_send(req, resp_str, strlen(resp_str));
    Pin1=1;
    motorpasos2();
    return ESP_OK;
}
static esp_err_t Pin1off_get_handler(httpd_req_t *req)
{
    const char* resp_str;
    if(Pin2==0) {resp_str = (const char*) HTMLWebPage00;}
    else {resp_str = (const char*) HTMLWebPage10;}
    httpd_resp_send(req, resp_str, strlen(resp_str));
    Pin1=0;
    motor();
    return ESP_OK;
}
static esp_err_t Pin2on_get_handler(httpd_req_t *req)
{
    const char* resp_str;
    if(Pin1==0) {resp_str = (const char*) HTMLWebPage10;}
    else {resp_str = (const char*) HTMLWebPage11;}
    httpd_resp_send(req, resp_str, strlen(resp_str));
    Pin2=1;
    motorpasos();
    return ESP_OK;
}
static esp_err_t Pin2off_get_handler(httpd_req_t *req)
{
    const char* resp_str;
    if(Pin1==0) {resp_str = (const char*) HTMLWebPage00;}
    else {resp_str = (const char*) HTMLWebPage01;}
    httpd_resp_send(req, resp_str, strlen(resp_str));
    Pin2=0;
    motor();
    return ESP_OK;
}

static const httpd_uri_t home = {
    .uri       = "/",                          //index 1=off, 2=off
    .method    = HTTP_GET,
    .handler   = home_get_handler,
    .user_ctx  = HTMLWebPage00                 
};
static const httpd_uri_t Pin1on = {
    .uri       = "/Pin1/on",                   //1 on
    .method    = HTTP_GET,
    .handler   = Pin1on_get_handler,
    .user_ctx  = HTMLWebPage00                    
};
static const httpd_uri_t Pin1off = {
    .uri       = "/Pin1/off",                   //1 off
    .method    = HTTP_GET,
    .handler   = Pin1off_get_handler,
    .user_ctx  = HTMLWebPage00                
};
static const httpd_uri_t Pin2on = {
    .uri       = "/Pin2/on",                   //2 on
    .method    = HTTP_GET,
    .handler   = Pin2on_get_handler,
    .user_ctx  = HTMLWebPage00                    
};
static const httpd_uri_t Pin2off = {
    .uri       = "/Pin2/off",                   //2 off
    .method    = HTTP_GET,
    .handler   = Pin2off_get_handler,
    .user_ctx  = HTMLWebPage00                
};

/* This handler allows the custom error handling functionality  */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } else if (strcmp("/echo", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &home);
        httpd_register_uri_handler(server, &Pin1on);
        httpd_register_uri_handler(server, &Pin1off);
        httpd_register_uri_handler(server, &Pin2on);
        httpd_register_uri_handler(server, &Pin2off);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

/******************************************** MAIN ********************************************/
void app_main(void)
{
    static httpd_handle_t server = NULL;

    // configure GPIOs, MOTOR A PASOS
	  gpio_pad_select_gpio(LEDP);
    gpio_set_direction(LEDP, GPIO_MODE_OUTPUT);
    gpio_set_level(LEDP, 0);// set initial status = OFF
   
	  gpio_pad_select_gpio(LEDP2);
    gpio_set_direction(LEDP2, GPIO_MODE_OUTPUT);
    gpio_set_level(LEDP2, 0);// set initial status = OFF
    
	  gpio_pad_select_gpio(LEDP3);
    gpio_set_direction(LEDP3, GPIO_MODE_OUTPUT);
    gpio_set_level(LEDP3, 0);// set initial status = OFF

	  gpio_pad_select_gpio(LEDP4);
    gpio_set_direction(LEDP4, GPIO_MODE_OUTPUT);
    gpio_set_level(LEDP4, 0);// set initial status = OFF




   // gpio_pad_select_gpio(Pin2IO);
   // gpio_set_direction(Pin2IO, GPIO_MODE_OUTPUT);
   //	gpio_set_level(Pin2IO, 0);// set initial status = OFF

    ESP_ERROR_CHECK(nvs_flash_init());  //Inicializa NVS flash
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

     ESP_ERROR_CHECK(example_connect());     //WiFi conexion segura

    // Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
    // and re-start it upon connection.
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    server = start_webserver();        // Start the server for the first time
}


void motorpasos(){
 int k;
   for (k=0;k<10;k++){
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

}

void motor(){

    gpio_set_level(LEDP,0);
      gpio_set_level(LEDP2,0);
      gpio_set_level(LEDP3,0);
      gpio_set_level(LEDP4,0); 
}



void motorpasos2(){
 int k;
   for (k=0;k<10;k++){
      gpio_set_level(LEDP,0);
      gpio_set_level(LEDP2,0);
      gpio_set_level(LEDP3,0);
      gpio_set_level(LEDP4,1);
      vTaskDelay(pdMS_TO_TICKS(25));
      gpio_set_level(LEDP,0);
      gpio_set_level(LEDP2,0);
      gpio_set_level(LEDP3,1);
      gpio_set_level(LEDP4,0);
      vTaskDelay(pdMS_TO_TICKS(25));
       gpio_set_level(LEDP,0);
      gpio_set_level(LEDP2,1);
      gpio_set_level(LEDP3,0);
      gpio_set_level(LEDP4,0);
      vTaskDelay(pdMS_TO_TICKS(25));
        gpio_set_level(LEDP,1);
      gpio_set_level(LEDP2,0);
      gpio_set_level(LEDP3,0);
      gpio_set_level(LEDP4,0);
      vTaskDelay(pdMS_TO_TICKS(25));

       }

}


