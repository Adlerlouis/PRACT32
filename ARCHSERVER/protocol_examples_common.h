/* Common functions for protocol examples, to establish Wi-Fi or Ethernet connection.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#pragma once
#define CONFIG_EXAMPLE_CONNECT_WIFI 1

//#define CONFIG_EXAMPLE_PROVIDE_WIFI_CONSOLE_CMD 1     //descomentar para interaccion con consola

#define CONFIG_EXAMPLE_WIFI_SSID                  "L_Adler"

#define CONFIG_EXAMPLE_WIFI_PASSWORD              "Axel_reyes_123"

#define CONFIG_EXAMPLE_WIFI_CONN_MAX_RETRY        6

#define CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD   -127  //-127 a 0

 

//seleccionar uno:

#define CONFIG_EXAMPLE_WIFI_SCAN_METHOD_FAST 1

//#define CONFIG_EXAMPLE_WIFI_SCAN_METHOD_ALL_CHANNEL 1

 

//seleccionar uno:

#define CONFIG_EXAMPLE_WIFI_CONNECT_AP_BY_SIGNAL 1

//#define CONFIG_EXAMPLE_WIFI_CONNECT_AP_BY_SECURITY

 

//seleccionar uno:

//#define CONFIG_EXAMPLE_WIFI_AUTH_OPEN 1

//#define CONFIG_EXAMPLE_WIFI_AUTH_WEP 1

//#define CONFIG_EXAMPLE_WIFI_AUTH_WPA_PSK 1

#define CONFIG_EXAMPLE_WIFI_AUTH_WPA2_PSK 1

//#define CONFIG_EXAMPLE_WIFI_AUTH_WPA_WPA2_PSK 1

//#define CONFIG_EXAMPLE_WIFI_AUTH_WPA2_ENTERPRISE 1

//#define CONFIG_EXAMPLE_WIFI_AUTH_WPA3_PSK 1

//#define CONFIG_EXAMPLE_WIFI_AUTH_WPA2_WPA3_PSK 1

//#define CONFIG_EXAMPLE_WIFI_AUTH_WAPI_PSK 1

 
#include "sdkconfig.h"
#include "esp_err.h"
#if !CONFIG_IDF_TARGET_LINUX
#include "esp_netif.h"
#if CONFIG_EXAMPLE_CONNECT_ETHERNET
#include "esp_eth.h"
#endif
#endif // !CONFIG_IDF_TARGET_LINUX

#ifdef __cplusplus
extern "C" {
#endif

#if !CONFIG_IDF_TARGET_LINUX
#if CONFIG_EXAMPLE_CONNECT_WIFI
#define EXAMPLE_NETIF_DESC_STA "example_netif_sta"
#endif

#if CONFIG_EXAMPLE_CONNECT_ETHERNET
#define EXAMPLE_NETIF_DESC_ETH "example_netif_eth"
#endif

/* Example default interface, prefer the ethernet one if running in example-test (CI) configuration */
#if CONFIG_EXAMPLE_CONNECT_ETHERNET
#define EXAMPLE_INTERFACE get_example_netif_from_desc(EXAMPLE_NETIF_DESC_ETH)
#define get_example_netif() get_example_netif_from_desc(EXAMPLE_NETIF_DESC_ETH)
#elif CONFIG_EXAMPLE_CONNECT_WIFI
#define EXAMPLE_INTERFACE get_example_netif_from_desc(EXAMPLE_NETIF_DESC_STA)
#define get_example_netif() get_example_netif_from_desc(EXAMPLE_NETIF_DESC_STA)
#endif

/**
 * @brief Configure Wi-Fi or Ethernet, connect, wait for IP
 *
 * This all-in-one helper function is used in protocols examples to
 * reduce the amount of boilerplate in the example.
 *
 * It is not intended to be used in real world applications.
 * See examples under examples/wifi/getting_started/ and examples/ethernet/
 * for more complete Wi-Fi or Ethernet initialization code.
 *
 * Read "Establishing Wi-Fi or Ethernet Connection" section in
 * examples/protocols/README.md for more information about this function.
 *
 * @return ESP_OK on successful connection
 */
esp_err_t example_connect(void);

/**
 * Counterpart to example_connect, de-initializes Wi-Fi or Ethernet
 */
esp_err_t example_disconnect(void);

/**
 * @brief Configure stdin and stdout to use blocking I/O
 *
 * This helper function is used in ASIO examples. It wraps installing the
 * UART driver and configuring VFS layer to use UART driver for console I/O.
 */
esp_err_t example_configure_stdin_stdout(void);

/**
 * @brief Returns esp-netif pointer created by example_connect() described by
 * the supplied desc field
 *
 * @param desc Textual interface of created network interface, for example "sta"
 * indicate default WiFi station, "eth" default Ethernet interface.
 *
 */
esp_netif_t *get_example_netif_from_desc(const char *desc);

#if CONFIG_EXAMPLE_PROVIDE_WIFI_CONSOLE_CMD
/**
 * @brief Register wifi connect commands
 *
 * Provide a simple wifi_connect command in esp_console.
 * This function can be used after esp_console is initialized.
 */
void example_register_wifi_connect_commands(void);
#endif

#if CONFIG_EXAMPLE_CONNECT_ETHERNET
/**
 * @brief Get the example Ethernet driver handle
 *
 * @return esp_eth_handle_t
 */
esp_eth_handle_t get_example_eth_handle(void);
#endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

#else
static inline esp_err_t example_connect(void) {return ESP_OK;}
#endif // !CONFIG_IDF_TARGET_LINUX

#ifdef __cplusplus
}
#endif


/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/*  Private Funtions of protocol example common */

#pragma once

#include "esp_err.h"
#include "esp_wifi.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_EXAMPLE_CONNECT_IPV6
#define MAX_IP6_ADDRS_PER_NETIF (5)

#if defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_LOCAL_LINK)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_LINK_LOCAL
#elif defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_GLOBAL)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_GLOBAL
#elif defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_SITE_LOCAL)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_SITE_LOCAL
#elif defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_UNIQUE_LOCAL)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_UNIQUE_LOCAL
#endif // if-elif CONFIG_EXAMPLE_CONNECT_IPV6_PREF_...

#endif


#if CONFIG_EXAMPLE_CONNECT_IPV6
extern const char *example_ipv6_addr_types_to_str[6];
#endif

void example_wifi_start(void);
void example_wifi_stop(void);
esp_err_t example_wifi_sta_do_connect(wifi_config_t wifi_config, bool wait);
esp_err_t example_wifi_sta_do_disconnect(void);
bool example_is_our_netif(const char *prefix, esp_netif_t *netif);
void example_print_all_netif_ips(const char *prefix);
void example_wifi_shutdown(void);
esp_err_t example_wifi_connect(void);
void example_ethernet_shutdown(void);
esp_err_t example_ethernet_connect(void);



#ifdef __cplusplus
}
#endif








/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/*  Private Funtions of protocol example common */

#pragma once

#include "esp_err.h"
#include "esp_wifi.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_EXAMPLE_CONNECT_IPV6
#define MAX_IP6_ADDRS_PER_NETIF (5)

#if defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_LOCAL_LINK)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_LINK_LOCAL
#elif defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_GLOBAL)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_GLOBAL
#elif defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_SITE_LOCAL)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_SITE_LOCAL
#elif defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_UNIQUE_LOCAL)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_UNIQUE_LOCAL
#endif // if-elif CONFIG_EXAMPLE_CONNECT_IPV6_PREF_...

#endif


#if CONFIG_EXAMPLE_CONNECT_IPV6
extern const char *example_ipv6_addr_types_to_str[6];
#endif

void example_wifi_start(void);
void example_wifi_stop(void);
esp_err_t example_wifi_sta_do_connect(wifi_config_t wifi_config, bool wait);
esp_err_t example_wifi_sta_do_disconnect(void);
bool example_is_our_netif(const char *prefix, esp_netif_t *netif);
void example_print_all_netif_ips(const char *prefix);
void example_wifi_shutdown(void);
esp_err_t example_wifi_connect(void);
void example_ethernet_shutdown(void);
esp_err_t example_ethernet_connect(void);



#ifdef __cplusplus
}
#endif
