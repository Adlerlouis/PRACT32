#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/param.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "rom/gpio.h"
#include "protocol_examples_common.h"
#include "file_serving_example_common.h"

int boton=0;
// Set LED GPIO
#define ledPin 2
#define LEDP 37
#define LEDP2 38
#define LEDP3 39
#define LEDP4 40
static const char *TAG="1LED Webserver";

// Set LED GPIO
#define ledPin  2
// Stores LED state
int ledState=0;
char dummy[20];
void motorpasos();
void motor();
const static char http_off_hml[] = "<meta content=\"width=device-width,initial-scale=1\" name=viewport><style>.container {display: flex; justify-content: space-between; align-items: center;}</style><div class=\"container\"><div><h1>MOTOR_OFF</h1><a href=on.html><img src=on.png></a></div><div><a href=off1.html><img src=off1.png></a></div></div>";
const static char http_on_hml[] = "<meta content=\"width=device-width,initial-scale=1\" name=viewport><style>.container {display: flex; justify-content: space-between; align-items: center;}</style><div class=\"container\"><div><h1>MOTOR_ON</h1><a href=off.html><img src=off.png></a></div><div><a href=on1.html><img src=on1.png></a></div></div>";

	

// embedded binary data
extern const uint8_t on_png_start[] asm("_binary_on_png_start");
extern const uint8_t on_png_end[]   asm("_binary_on_png_end");
extern const uint8_t off_png_start[] asm("_binary_off_png_start");
extern const uint8_t off_png_end[]   asm("_binary_off_png_end");




// Max length a file path can have on storage
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

// Max size of an individual file. Make sure this value is same as that set in upload_script.html
#define MAX_FILE_SIZE   (200*1024) // 200 KB
#define MAX_FILE_SIZE_STR "200KB"
#define SCRATCH_BUFSIZE  8192               // Scratch buffer size

struct file_server_data {
    char base_path[ESP_VFS_PATH_MAX + 1];   // Base path of file storage
    char scratch[SCRATCH_BUFSIZE];          // Scratch buffer for temporary storage during file transfer
};

#define IS_FILE_EXT(filename, ext)(strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)


// Handler to respond with an icon file embedded in flash.
// Browsers expect to GET website icon at URI /favicon.ico.
// This can be overridden by uploading file with same name */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}

// Handler to download a file kept on the server
static esp_err_t get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
   

    //----------------- Copies the full path into destination buffer ----------------------------
    char *dest = filepath;
    const char *filename = ((struct file_server_data *)req->user_ctx)->base_path;  //base path
    const char *uri = req->uri;
    size_t destsize = sizeof(filepath);
    const size_t base_pathlen = strlen(filename); //strlen(base_path);
    size_t pathlen = strlen(uri);
    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {

        filename= NULL; // Full path string won't fit into destination buffer
    }
    // Construct full path (base + path)
    strcpy(dest, filename); //base path
    strlcpy(dest + base_pathlen, uri, pathlen + 1);
    filename= dest + base_pathlen; //pointer to path, skipping the base
    if (!filename) {
        ESP_LOGE(TAG, "Filename is too long");
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    ESP_LOGE(TAG, "URI Filename : %s",  filename);

    //----------------- GET cases --------------------------------------------------------------
    // If name has trailing '/', respond with default root
    if (filename[strlen(filename) - 1] == '/')  //raiz
    {
        httpd_resp_set_type(req, "text/html");
        if(ledState==0){ httpd_resp_send(req, (const char *)http_off_hml, sizeof(http_off_hml)); }
        else { httpd_resp_send(req, (const char *)http_on_hml, sizeof(http_on_hml)); }
    }
    else if (strcmp(filename, "/on.html") == 0)               // ON page
    {
        ledState=1;
        gpio_set_level(ledPin, 1);
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, (const char *)http_on_hml, sizeof(http_on_hml));
    }
    else if (strcmp(filename, "/off.html") == 0)         // OFF page
    {
        ledState=0;
        
        gpio_set_level(ledPin, 0);
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, (const char *)http_off_hml, sizeof(http_off_hml));
    }
    else if (strcmp(filename, "/on.png") == 0)          // ON image
    {
        const size_t on_png_size = (on_png_end - on_png_start);
        httpd_resp_set_type(req, "image/png");
        httpd_resp_send(req, (const char *)on_png_start, on_png_size);
    }
    else if (strcmp(filename, "/off.png") == 0)         // OFF image
    {
        const size_t off_png_size = (off_png_end - off_png_start);
        httpd_resp_set_type(req, "image/png");
        httpd_resp_send(req, (const char *)off_png_start, off_png_size);
    }
    else if (strcmp(filename, "/favicon.ico") == 0)
    {
        ESP_LOGE(TAG, "Favicon OK");
        return favicon_get_handler(req);
    }
    
    
    else if (strcmp(filename, "/on1.html") == 0)               // ON page
    {
        motorpasos();
        gpio_set_level(ledPin, 1);
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, (const char *)http_on_hml, sizeof(http_on_hml));
    }
    else if (strcmp(filename, "/off1.html") == 0)         // OFF page
    {
        motor();
        gpio_set_level(ledPin, 0);
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, (const char *)http_off_hml, sizeof(http_off_hml));
    }
    else if (strcmp(filename, "/on1.png") == 0)          // ON image
    {
        const size_t on_png_size = (on_png_end - on_png_start);
        httpd_resp_set_type(req, "image/png");
        httpd_resp_send(req, (const char *)on_png_start, on_png_size);
    }
    else if (strcmp(filename, "/off1.png") == 0)         // OFF image
    {
        const size_t off_png_size = (off_png_end - off_png_start);
        httpd_resp_set_type(req, "image/png");
        httpd_resp_send(req, (const char *)off_png_start, off_png_size);
    }
    else if (strcmp(filename, "/favicon.ico") == 0)
    {
        ESP_LOGE(TAG, "Favicon OK");
        return favicon_get_handler(req);
    }
    // URI not present
    else
    {
        ESP_LOGE(TAG, "URI not found : %s", filepath);
        // Respond with 404 Not Found
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI does not exist");
        return ESP_FAIL;
    }



    // Respond with an empty chunk to signal HTTP response completion
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// Function to start the file server
esp_err_t start_server(const char *base_path)
{
    static struct file_server_data *server_data = NULL;

    /* Validate file storage base path */
    if (!base_path || strcmp(base_path, "/spiffs") != 0) {
        ESP_LOGE(TAG, "File server presently supports only '/spiffs' as base path");
        return ESP_ERR_INVALID_ARG;
    }

    if (server_data) {
        ESP_LOGE(TAG, "File server already started");
        return ESP_ERR_INVALID_STATE;
    }

    /* Allocate memory for server data */
    server_data = calloc(1, sizeof(struct file_server_data));
    if (!server_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return ESP_ERR_NO_MEM;
    }
    strlcpy(server_data->base_path, base_path,
            sizeof(server_data->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    // URI handler for getting uploaded files
    httpd_uri_t file_download = {
        .uri       = "/*",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = get_handler,
        .user_ctx  = server_data    // Pass server data as context
    };
    httpd_register_uri_handler(server, &file_download);

    return ESP_OK;
}



/********************************************** MAIN *****************************************/
void app_main(void)
{
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



    ESP_ERROR_CHECK(nvs_flash_init());  //Inicializa NVS flash
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());     //WiFi conexion segura
   

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */


    /* Start the file server */
    ESP_ERROR_CHECK(start_server("/spiffs"));
}





void motorpasos(){
 int k;
   for (k=0;k<20;k++){
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



