
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
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "file_serving_example_common.h"
#include "dht.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/adc.h" // Para el ADC
#include "esp_adc_cal.h" // Para calibrar el ADC

static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
static const gpio_num_t dht_gpio =47;
int16_t temperature = 0;
int16_t humidity = 0;
float temperatura=0;
float humedad=0;
char buffer [50];
int n=0;
int pot;//DECALRAMOS UN CONTADOR PARA PODER MOSTRAR EN PANTALLA
adc_channel_t  adc_pot  = ADC1_CHANNEL_1;   //DECLARAMOS EL PORT GPIO DE EL ADC
void init_hw(void);//FUNCION ÁRA LLAMAR A LOS PERIFERICOS 
static const char *TAG="Weatherstation Webserver";




/********************************************** SPIFFS *****************************************/
static esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,   // This decides the maximum number of files that can be created on the storage
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

/********************************************** SERVER *****************************************/
// embedded binary data
extern const uint8_t Page_html_start[] asm("_binary_Page_html_start");
extern const uint8_t Page_html_end[]   asm("_binary_Page_html_end");


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
    FILE *fd = NULL;
    struct stat file_stat;
   /* 
    pot = adc1_get_raw(adc_pot);
    char adc_value_str[20];  // Suficientemente grande para contener el valor del ADC
    sprintf(adc_value_str, "%d", pot);
*/
 

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
        httpd_resp_set_type(req, "text/html");              //Enviar formato
        const size_t Page_html_size = (Page_html_end - Page_html_start);
        httpd_resp_send_chunk(req, (const char *)Page_html_start, Page_html_size);
    }
    else if (strcmp(filename, "/readADC") == 0)               // Enviar valores de graficos
    {

        httpd_resp_set_type(req, "text/xml"); //humedad
        httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
        httpd_resp_sendstr_chunk(req, "<?xml version = \"1.0\" ?>");
        httpd_resp_sendstr_chunk(req, "<inputs>");
        httpd_resp_sendstr_chunk(req, "<analog>");
        n=sprintf (buffer, "%f", temperatura);
        httpd_resp_sendstr_chunk(req, buffer); //temperatura
        httpd_resp_sendstr_chunk(req, "</analog>");
        httpd_resp_sendstr_chunk(req, "<analog>");
        n=sprintf (buffer, "%f", humedad);
        /*pot = adc1_get_raw(adc_pot); //luminosidad
        httpd_resp_sendstr_chunk(req, "</analog>");
        httpd_resp_sendstr_chunk(req, "<analog>");
        n=sprintf	(buffer, "%d", pot);
      
      */
  }

    else if (strcmp(filename, "/favicon.ico") == 0)
    {
        ESP_LOGE(TAG, "Favicon OK");
        return favicon_get_handler(req);
    }
    
    else if (strcmp(filename, "/fonts/digital-7-mono.ttf") == 0)
    {
        // If file not present on SPIFFS check if URI corresponds to one of the hardcoded paths
        if (stat(filepath, &file_stat) == -1) 
        {
            ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
            // Respond with 404 Not Found 
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
            return ESP_FAIL;
        }
        fd = fopen(filepath, "r");
        ESP_LOGE(TAG, "Read existing file : %s", filepath);
        if (!fd) {
            ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
            // Respond with 500 Internal Server Error 
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
            return ESP_FAIL;
        }

        //Set HTTP response content type according to file extension
        ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filename, file_stat.st_size);
        if (IS_FILE_EXT(filename, ".pdf")) { httpd_resp_set_type(req, "application/pdf");    } 
        else if (IS_FILE_EXT(filename, ".html")) { httpd_resp_set_type(req, "text/html");    } 
        else if (IS_FILE_EXT(filename, ".jpeg")) { httpd_resp_set_type(req, "image/jpeg");   } 
        else if (IS_FILE_EXT(filename, ".ico"))  { httpd_resp_set_type(req, "image/x-icon"); } 
        else if (IS_FILE_EXT(filename, ".css"))  { httpd_resp_set_type(req, "text/css");     } 
        else {
        // This is a limited set only For any other type always set as plain text 
            httpd_resp_set_type(req, "text/plain");
        }
        //----------------- Retrieve the pointer to scratch buffer for temporary storage -----------------
        char *chunk = ((struct file_server_data *)req->user_ctx)->scratch;
        size_t chunksize;
        do {
            /* Read file in chunks into the scratch buffer */
            chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);

            if (chunksize > 0) {
                /* Send the buffer contents as HTTP response chunk */
                if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                    fclose(fd);
                    ESP_LOGE(TAG, "File sending failed!");
                    /* Abort sending file */
                    httpd_resp_sendstr_chunk(req, NULL);
                    /* Respond with 500 Internal Server Error */
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
                }
            }

            // Keep looping till the whole file is sent 
        } while (chunksize != 0);

        // Close file after sending complete 
        fclose(fd);
        ESP_LOGI(TAG, "File sending complete");
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
esp_err_t start_file_server(const char *base_path)
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
       /* FLASH Init for MAC */



    ESP_ERROR_CHECK(nvs_flash_init());  //Inicializa NVS flash

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());     //WiFi coneUxion segura

   
 /* Start the file server */
    ESP_ERROR_CHECK(start_file_server("/spiffs"));

    init_hw();
     while(1) {

    //pot=adc1_get_raw(adc_pot);//Manda un echo al puerto para ver si valores 
    printf("ADC MUESTRA: %d\n",pot);//muestreo del ADC
        if (dht_read_data(sensor_type, dht_gpio, &humidity, &temperature) == ESP_OK)
            printf("Humidity: %d%% Temp: %d^C\n", humidity / 10, temperature / 10);
        else
            printf("Could not read data from sensor\n");
        temperatura=temperature; temperatura=temperatura/10; 
        humedad=humidity; humedad=humedad/10;
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

	void init_hw(void){
    adc1_config_width(ADC_BITWIDTH_12); //ESCALA DEL ADC(ADC_ESCALA)
    adc1_config_channel_atten(adc_pot,ADC_ATTEN_DB_11); //AMPLITUD DE PASO DE ADC(VARIABLE_DE_MUESTREO,ESCALA_DE_PASO) 
}	
