#include "network_manager.h"
#include "config_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "NET_MGR";
static wifi_mode_t current_mode = WIFI_MODE_AP;
static char local_ip[16] = SOFTAP_IP;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " connected", MAC2STR(event->mac));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " disconnected", MAC2STR(event->mac));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Station mode started, connecting...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Station disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(local_ip, sizeof(local_ip), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", local_ip);
    }
}

void network_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing Network Manager");
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, 
        ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
        IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    network_manager_start_softap("ESP32-MIDI-Gateway", "midi1234");
    
    ESP_LOGI(TAG, "Network Manager initialized (SoftAP: %s)", SOFTAP_IP);
}

void network_manager_start_softap(const char *ssid, const char *password)
{
    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = 8,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    if (password && strlen(password) > 0) {
        strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password) - 1);
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    strncpy(local_ip, SOFTAP_IP, sizeof(local_ip) - 1);
    local_ip[sizeof(local_ip) - 1] = '\0';
    current_mode = WIFI_MODE_AP;
    ESP_LOGI(TAG, "SoftAP started - SSID: %s", ssid);
}

void network_manager_start_station(const char *ssid, const char *password)
{
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_connect();
    current_mode = WIFI_MODE_STA;
    ESP_LOGI(TAG, "Station mode connecting to SSID: %s", ssid);
}

void network_manager_set_mode(wifi_mode_t mode)
{
    switch (mode) {
        case WIFI_MODE_AP:
            current_mode = WIFI_MODE_AP;
            break;
        case WIFI_MODE_STA:
            current_mode = WIFI_MODE_STA;
            break;
        case WIFI_MODE_APSTA:
            current_mode = WIFI_MODE_APSTA;
            break;
        default:
            break;
    }
}

wifi_mode_t network_manager_get_mode(void)
{
    return current_mode;
}

const char *network_manager_get_ip(void)
{
    return local_ip;
}

int network_manager_get_sta_list(char output[][20], int max_count)
{
    wifi_sta_list_t sta_list;
    esp_err_t err = esp_wifi_ap_get_sta_list(&sta_list);
    if (err != ESP_OK) {
        return 0;
    }
    
    int count = (sta_list.num > max_count) ? max_count : sta_list.num;
    for (int i = 0; i < count; i++) {
        snprintf(output[i], 20, MACSTR, MAC2STR(sta_list.sta[i].mac));
    }
    return count;
}