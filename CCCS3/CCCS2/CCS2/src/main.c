
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
#include "dht.h"
#include "protocol_examples_common.h"
#include "file_serving_example_common.h"
#include "dht.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/adc.h" // Para el adc
#include "esp_adc_cal.h" // Para calibrar el ADC
static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
static const gpio_num_t dht_gpio = 47;

int16_t temperature = 0;
int16_t humidity = 0;
float temperatura=0;
float humedad=0;
char buffer [50];
int n=0;
#define ledPin 2
#define DETECTOR 45
static const char *TAG="Weatherstation Webserver tipo App";
int  pot;//DECALRAMOS UN CONTADOR PARA PODER MOSTRAR EN PANTALLA
adc_channel_t  adc_pot  = ADC1_CHANNEL_1;   //DECLARAMOS EL PORT GPIO DE EL ADC
void init_hw(void);//FUNCION ÁRA LLAMAR A LOS PERIFERICOS 






/********************************************** SERVER *****************************************/
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
    //FILE *fd = NULL;
    //struct stat file_stat;
    
  
    pot = adc1_get_raw(adc_pot);
    char adc_value_str[20];  // Suficientemente grande para contener el valor del ADC
    sprintf(adc_value_str, "%d", pot);
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
        //httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
     
        //------------------- SIN ESTILO ------------------------------
        /*httpd_resp_sendstr_chunk(req, "<!DOCTYPE html> <html>");
        httpd_resp_sendstr_chunk(req, "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">");
        httpd_resp_sendstr_chunk(req, "<title>ESP32 Weather Station</title>");
        httpd_resp_sendstr_chunk(req, "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
        httpd_resp_sendstr_chunk(req, "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}");
        httpd_resp_sendstr_chunk(req, "p {font-size: 24px;color: #444444;margin-bottom: 10px;}");
        httpd_resp_sendstr_chunk(req, "</style>");
        httpd_resp_sendstr_chunk(req, "</head>");
        httpd_resp_sendstr_chunk(req, "<body>");
        httpd_resp_sendstr_chunk(req, "<div id=\"webpage\">");
        httpd_resp_sendstr_chunk(req, "<h1>ESP32 Weather Station</h1>");
        httpd_resp_sendstr_chunk(req, "<p>Temperature: ");
        n=sprintf (buffer, "%f", temperatura);
        httpd_resp_sendstr_chunk(req, buffer);
        httpd_resp_sendstr_chunk(req, "&deg;C</p>");
        httpd_resp_sendstr_chunk(req, "<p>Humidity: ");
        n=sprintf (buffer, "%f", humedad);
        httpd_resp_sendstr_chunk(req, buffer);
        httpd_resp_sendstr_chunk(req, "%</p>");
        httpd_resp_sendstr_chunk(req, "</div>");
        httpd_resp_sendstr_chunk(req, "</body>");
        httpd_resp_sendstr_chunk(req, "</html>");*/

        //---------------- CON ESTILO ------------------------
        httpd_resp_sendstr_chunk(req,"<!DOCTYPE html>");
        httpd_resp_sendstr_chunk(req,"<html>");
        httpd_resp_sendstr_chunk(req,"<head>");
        httpd_resp_sendstr_chunk(req,"<title>ESP32 Weather Station</title>");
        httpd_resp_sendstr_chunk(req,"<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
        httpd_resp_sendstr_chunk(req,"<link href='https://fonts.googleapis.com/css?family=Open+Sans:300,400,600' rel='stylesheet'>");
        httpd_resp_sendstr_chunk(req,"<style>");
        httpd_resp_sendstr_chunk(req,"html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #444444;}");
        httpd_resp_sendstr_chunk(req,"body{margin: 0px;} ");
        httpd_resp_sendstr_chunk(req,"h1 {margin: 50px auto 30px;} ");
        httpd_resp_sendstr_chunk(req,".side-by-side{display: table-cell;vertical-align: middle;position: relative;}");
        httpd_resp_sendstr_chunk(req,".text{font-weight: 600;font-size: 19px;width: 200px;}");
        httpd_resp_sendstr_chunk(req,".reading{font-weight: 300;font-size: 50px;padding-right: 25px;}");
        httpd_resp_sendstr_chunk(req,".temperature .reading{color: #F29C1F;}");
        httpd_resp_sendstr_chunk(req,".humidity .reading{color: #3B97D3;}");
        httpd_resp_sendstr_chunk(req,".pressure .reading{color: #26B99A;}");
        httpd_resp_sendstr_chunk(req,".altitude .reading{color: #955BA5;}");
        httpd_resp_sendstr_chunk(req,".superscript{font-size: 17px;font-weight: 600;position: absolute;top: 10px;}");
        httpd_resp_sendstr_chunk(req,".data{padding: 10px;}");
        httpd_resp_sendstr_chunk(req,".container{display: table;margin: 0 auto;}");
        httpd_resp_sendstr_chunk(req,".icon{width:65px}");
        httpd_resp_sendstr_chunk(req,"</style>");
        httpd_resp_sendstr_chunk(req,"</head>");
        httpd_resp_sendstr_chunk(req,"<body>");
        httpd_resp_sendstr_chunk(req,"<h1>ESP32 Weather Station</h1>");
        httpd_resp_sendstr_chunk(req,"<div class='container'>");
        httpd_resp_sendstr_chunk(req,"<div class='data temperature'>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side icon'>");
        httpd_resp_sendstr_chunk(req,"<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982");
        httpd_resp_sendstr_chunk(req,"C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718");
        httpd_resp_sendstr_chunk(req,"c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833");
        httpd_resp_sendstr_chunk(req,"c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22");
        httpd_resp_sendstr_chunk(req,"s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>");
        httpd_resp_sendstr_chunk(req,"</div>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side text'>Temperature</div>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side reading'>");
        n=sprintf (buffer, "%f", temperatura);
        httpd_resp_sendstr_chunk(req, buffer);
        //httpd_resp_sendstr_chunk(req,(int)temperature;
        httpd_resp_sendstr_chunk(req,"<span class='superscript'>&deg;C</span></div>");
        httpd_resp_sendstr_chunk(req,"</div>");
        httpd_resp_sendstr_chunk(req,"<div class='data humidity'>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side icon'>");
        httpd_resp_sendstr_chunk(req,"<svg enable-background='new 0 0 29.235 40.64'height=40.64px id=Layer_1 version=1.1 viewBox='0 0 29.235 40.64'width=29.235px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><path d='M14.618,0C14.618,0,0,17.95,0,26.022C0,34.096,6.544,40.64,14.618,40.64s14.617-6.544,14.617-14.617");
        httpd_resp_sendstr_chunk(req,"C29.235,17.95,14.618,0,14.618,0z M13.667,37.135c-5.604,0-10.162-4.56-10.162-10.162c0-0.787,0.638-1.426,1.426-1.426");
        httpd_resp_sendstr_chunk(req,"c0.787,0,1.425,0.639,1.425,1.426c0,4.031,3.28,7.312,7.311,7.312c0.787,0,1.425,0.638,1.425,1.425");
        httpd_resp_sendstr_chunk(req,"C15.093,36.497,14.455,37.135,13.667,37.135z'fill=#3C97D3 /></svg>");
        httpd_resp_sendstr_chunk(req,"</div>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side text'>Humidity</div>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side reading'>");
        n=sprintf (buffer, "%f", humedad);
        httpd_resp_sendstr_chunk(req, buffer);
        //httpd_resp_sendstr_chunk(req,(int)humidity;
        httpd_resp_sendstr_chunk(req,"<span class='superscript'>%</span></div>");
        httpd_resp_sendstr_chunk(req,"</div>");
        httpd_resp_sendstr_chunk(req,"<div class='data pressure'>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side icon'>");
        httpd_resp_sendstr_chunk(req,"<svg enable-background='new 0 0 40.542 40.541'height=40.541px id=Layer_1 version=1.1 viewBox='0 0 40.542 40.541'width=40.542px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M34.313,20.271c0-0.552,0.447-1,1-1h5.178c-0.236-4.841-2.163-9.228-5.214-12.593l-3.425,3.424");
        httpd_resp_sendstr_chunk(req,"c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293c-0.391-0.391-0.391-1.023,0-1.414l3.425-3.424");
        httpd_resp_sendstr_chunk(req,"c-3.375-3.059-7.776-4.987-12.634-5.215c0.015,0.067,0.041,0.13,0.041,0.202v4.687c0,0.552-0.447,1-1,1s-1-0.448-1-1V0.25");
        httpd_resp_sendstr_chunk(req,"c0-0.071,0.026-0.134,0.041-0.202C14.39,0.279,9.936,2.256,6.544,5.385l3.576,3.577c0.391,0.391,0.391,1.024,0,1.414");
        httpd_resp_sendstr_chunk(req,"c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293L5.142,6.812c-2.98,3.348-4.858,7.682-5.092,12.459h4.804");
        httpd_resp_sendstr_chunk(req,"c0.552,0,1,0.448,1,1s-0.448,1-1,1H0.05c0.525,10.728,9.362,19.271,20.22,19.271c10.857,0,19.696-8.543,20.22-19.271h-5.178");
        httpd_resp_sendstr_chunk(req,"C34.76,21.271,34.313,20.823,34.313,20.271z M23.084,22.037c-0.559,1.561-2.274,2.372-3.833,1.814");
        httpd_resp_sendstr_chunk(req,"c-1.561-0.557-2.373-2.272-1.815-3.833c0.372-1.041,1.263-1.737,2.277-1.928L25.2,7.202L22.497,19.05");
        httpd_resp_sendstr_chunk(req,"C23.196,19.843,23.464,20.973,23.084,22.037z'fill=#26B999 /></g></svg>");
        httpd_resp_sendstr_chunk(req,"</div>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side text'>luminosidad</div>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side reading'>");
        //httpd_resp_sendstr_chunk(req,"(int)luminosidad");
        n=sprintf (buffer, "%f", (pot*1.0)); 
        httpd_resp_sendstr_chunk(req, buffer);
        /*httpd_resp_sendstr_chunk(req,"<span class='superscript'>hPa</span></div>");
        httpd_resp_sendstr_chunk(req,"</div>");
        httpd_resp_sendstr_chunk(req,"<div class='data altitude'>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side icon'>");
        httpd_resp_sendstr_chunk(req,"<svg enable-background='new 0 0 58.422 40.639'height=40.639px id=Layer_1 version=1.1 viewBox='0 0 58.422 40.639'width=58.422px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M58.203,37.754l0.007-0.004L42.09,9.935l-0.001,0.001c-0.356-0.543-0.969-0.902-1.667-0.902");
        httpd_resp_sendstr_chunk(req,"c-0.655,0-1.231,0.32-1.595,0.808l-0.011-0.007l-0.039,0.067c-0.021,0.03-0.035,0.063-0.054,0.094L22.78,37.692l0.008,0.004");
        httpd_resp_sendstr_chunk(req,"c-0.149,0.28-0.242,0.594-0.242,0.934c0,1.102,0.894,1.995,1.994,1.995v0.015h31.888c1.101,0,1.994-0.893,1.994-1.994");
        httpd_resp_sendstr_chunk(req,"C58.422,38.323,58.339,38.024,58.203,37.754z'fill=#955BA5 /><path d='M19.704,38.674l-0.013-0.004l13.544-23.522L25.13,1.156l-0.002,0.001C24.671,0.459,23.885,0,22.985,0");
        httpd_resp_sendstr_chunk(req,"c-0.84,0-1.582,0.41-2.051,1.038l-0.016-0.01L20.87,1.114c-0.025,0.039-0.046,0.082-0.068,0.124L0.299,36.851l0.013,0.004");
        httpd_resp_sendstr_chunk(req,"C0.117,37.215,0,37.62,0,38.059c0,1.412,1.147,2.565,2.565,2.565v0.015h16.989c-0.091-0.256-0.149-0.526-0.149-0.813");
        httpd_resp_sendstr_chunk(req,"C19.405,39.407,19.518,39.019,19.704,38.674z'fill=#955BA5 /></g></svg>");
        httpd_resp_sendstr_chunk(req,"</div>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side text'>Altitude</div>");
        httpd_resp_sendstr_chunk(req,"<div class='side-by-side reading'>");
        httpd_resp_sendstr_chunk(req,"(int)altitude");
        httpd_resp_sendstr_chunk(req,"<span class='superscript'>m</span></div>");
       */
        httpd_resp_sendstr_chunk(req,"</div>");
        httpd_resp_sendstr_chunk(req,"</div>");
        httpd_resp_sendstr_chunk(req,"</body>");
        httpd_resp_sendstr_chunk(req,"</html>");
    }

    else if (strcmp(filename, "/favicon.ico") == 0)
    {
        ESP_LOGE(TAG, "Favicon OK");
        return favicon_get_handler(req);
    }
    else    // URI Error
    {
        ESP_LOGE(TAG, "URI error");
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

    esp_rom_gpio_pad_select_gpio(DETECTOR);
    gpio_set_direction(DETECTOR, GPIO_MODE_INPUT);
    gpio_set_level(DETECTOR , 0);   // set initial status = OFF
 
    ESP_ERROR_CHECK(nvs_flash_init());  //Inicializa NVS flash

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());     //WiFi coneUxion segura
   
    init_hw();
   

    /* Start the file server */
    ESP_ERROR_CHECK(start_file_server("/spiffs"));

    
    while(1) {
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
