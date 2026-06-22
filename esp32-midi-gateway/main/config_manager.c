#include "config_manager.h"
#include "esp_log.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "CONFIG_MGR";

void config_manager_get_default(gateway_config_t *config)
{
    memset(config, 0, sizeof(gateway_config_t));
    strcpy(config->wifi_ssid, "ESP32-MIDI-Gateway");
    strcpy(config->wifi_password, "midi1234");
    config->softap_mode = true;
    config->midi_input = MIDI_INPUT_AUTO;
    config->midi_output = MIDI_OUTPUT_BROADCAST;
    strcpy(config->udp_target_ip, "255.255.255.255");
    config->udp_port = MIDI_UDP_PORT_DEFAULT;
    config->encryption = ENCRYPT_NONE;
    config->auth_mode = AUTH_IP_WHITELIST;
    strcpy(config->auth_key, "");
    strcpy(config->ip_whitelist, "192.168.3.0/24");
    strcpy(config->ssh_password, "XUE2026");
    strcpy(config->sensitive_password, "XUE2026");
    memset(config->ble_device_name, 0, sizeof(config->ble_device_name));
    memset(config->ble_device_addr, 0, sizeof(config->ble_device_addr));
    config->ble_paired = false;
    config->emergency_brake = false;
}

void config_manager_init(void)
{
    ESP_LOGI(TAG, "Config manager initialized");
}

void config_manager_load(gateway_config_t *config)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &handle);
    
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open failed (%s), loading defaults", esp_err_to_name(err));
        config_manager_get_default(config);
        return;
    }
    
    size_t len = sizeof(gateway_config_t);
    err = nvs_get_blob(handle, "gateway_cfg", config, &len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No config found (%s), loading defaults", esp_err_to_name(err));
        config_manager_get_default(config);
    }
    
    nvs_close(handle);
}

void config_manager_save(const gateway_config_t *config)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open for write failed: %s", esp_err_to_name(err));
        return;
    }
    
    err = nvs_set_blob(handle, "gateway_cfg", config, sizeof(gateway_config_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS set blob failed: %s", esp_err_to_name(err));
    } else {
        err = nvs_commit(handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Configuration saved successfully");
        }
    }
    
    nvs_close(handle);
}

void config_manager_factory_reset(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
        ESP_LOGW(TAG, "Factory reset performed - all config erased");
    }
}