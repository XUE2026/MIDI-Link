#include "config_manager.h"
#include "esp_log.h"
#include "nvs.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "CONFIG_MGR";

static void generate_random_password(char *buf, size_t buf_len)
{
    if (buf == NULL || buf_len < 8) {
        return;
    }

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%";
    uint8_t random_bytes[32];
    esp_fill_random(random_bytes, sizeof(random_bytes));

    size_t charset_len = strlen(charset);
    size_t pwd_len = (buf_len - 1 < 16) ? (buf_len - 1) : 16;

    for (size_t i = 0; i < pwd_len; i++) {
        buf[i] = charset[random_bytes[i % sizeof(random_bytes)] % charset_len];
    }
    buf[pwd_len] = '\0';
}

static bool is_first_boot(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return true;
    }

    uint8_t flag = 0;
    size_t len = sizeof(flag);
    err = nvs_get_u8(handle, "first_boot_done", &flag, &len);
    nvs_close(handle);

    return (err != ESP_OK || flag == 0);
}

static void mark_first_boot_done(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        nvs_set_u8(handle, "first_boot_done", 1);
        nvs_commit(handle);
        nvs_close(handle);
    }
}

void config_manager_get_default(gateway_config_t *config)
{
    memset(config, 0, sizeof(gateway_config_t));
    strncpy(config->wifi_ssid, "ESP32-MIDI-Gateway", sizeof(config->wifi_ssid) - 1);
    config->wifi_ssid[sizeof(config->wifi_ssid) - 1] = '\0';
    strncpy(config->wifi_password, "midi1234", sizeof(config->wifi_password) - 1);
    config->wifi_password[sizeof(config->wifi_password) - 1] = '\0';
    config->softap_mode = true;
    config->midi_input = MIDI_INPUT_AUTO;
    config->midi_output = MIDI_OUTPUT_BROADCAST;
    strncpy(config->udp_target_ip, "255.255.255.255", sizeof(config->udp_target_ip) - 1);
    config->udp_target_ip[sizeof(config->udp_target_ip) - 1] = '\0';
    config->udp_port = MIDI_UDP_PORT_DEFAULT;
    config->encryption = ENCRYPT_NONE;
    config->auth_mode = AUTH_IP_WHITELIST;
    config->auth_key[0] = '\0';
    strncpy(config->ip_whitelist, "192.168.3.0/24", sizeof(config->ip_whitelist) - 1);
    config->ip_whitelist[sizeof(config->ip_whitelist) - 1] = '\0';
    strncpy(config->ssh_password, "XUE2026", sizeof(config->ssh_password) - 1);
    config->ssh_password[sizeof(config->ssh_password) - 1] = '\0';
    strncpy(config->sensitive_password, "XUE2026", sizeof(config->sensitive_password) - 1);
    config->sensitive_password[sizeof(config->sensitive_password) - 1] = '\0';
    memset(config->ble_device_name, 0, sizeof(config->ble_device_name));
    memset(config->ble_device_addr, 0, sizeof(config->ble_device_addr));
    config->ble_paired = false;
    config->emergency_brake = false;
}

void config_manager_init(void)
{
    ESP_LOGI(TAG, "Config manager initializing");

    if (is_first_boot()) {
        ESP_LOGW(TAG, "First boot detected - generating random credentials");

        gateway_config_t config;
        config_manager_get_default(&config);

        char random_pwd[16];
        generate_random_password(random_pwd, sizeof(random_pwd));
        strncpy(config.wifi_password, random_pwd, sizeof(config.wifi_password) - 1);
        config.wifi_password[sizeof(config.wifi_password) - 1] = '\0';

        char random_sensitive[32];
        generate_random_password(random_sensitive, sizeof(random_sensitive));
        strncpy(config.sensitive_password, random_sensitive, sizeof(config.sensitive_password) - 1);
        config.sensitive_password[sizeof(config.sensitive_password) - 1] = '\0';

        char random_ssh[32];
        generate_random_password(random_ssh, sizeof(random_ssh));
        strncpy(config.ssh_password, random_ssh, sizeof(config.ssh_password) - 1);
        config.ssh_password[sizeof(config.ssh_password) - 1] = '\0';

        config_manager_save(&config);
        mark_first_boot_done();

        ESP_LOGW(TAG, "========================================");
        ESP_LOGW(TAG, "  FIRST BOOT CREDENTIALS");
        ESP_LOGW(TAG, "  WiFi AP: %s", config.wifi_ssid);
        ESP_LOGW(TAG, "  WiFi Password: %s", config.wifi_password);
        ESP_LOGW(TAG, "  Admin Password: %s", config.sensitive_password);
        ESP_LOGW(TAG, "  (Please change these after first login)");
        ESP_LOGW(TAG, "========================================");
    }

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

#define CONFIG_SAVE_MAX_RETRIES 3
#define CONFIG_SAVE_RETRY_DELAY_MS 50

bool config_manager_save(const gateway_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    for (int attempt = 0; attempt < CONFIG_SAVE_MAX_RETRIES; attempt++) {
        nvs_handle_t handle;
        esp_err_t err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "NVS open for write attempt %d failed: %s",
                     attempt + 1, esp_err_to_name(err));
            if (attempt < CONFIG_SAVE_MAX_RETRIES - 1) {
                vTaskDelay(pdMS_TO_TICKS(CONFIG_SAVE_RETRY_DELAY_MS));
            }
            continue;
        }

        err = nvs_set_blob(handle, "gateway_cfg", config, sizeof(gateway_config_t));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "NVS set blob attempt %d failed: %s",
                     attempt + 1, esp_err_to_name(err));
            nvs_close(handle);
            if (attempt < CONFIG_SAVE_MAX_RETRIES - 1) {
                vTaskDelay(pdMS_TO_TICKS(CONFIG_SAVE_RETRY_DELAY_MS));
            }
            continue;
        }

        err = nvs_commit(handle);
        nvs_close(handle);

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Configuration saved successfully (attempt %d)", attempt + 1);
            return true;
        }

        ESP_LOGE(TAG, "NVS commit attempt %d failed: %s",
                 attempt + 1, esp_err_to_name(err));
        if (attempt < CONFIG_SAVE_MAX_RETRIES - 1) {
            vTaskDelay(pdMS_TO_TICKS(CONFIG_SAVE_RETRY_DELAY_MS));
        }
    }

    ESP_LOGE(TAG, "Config save failed after %d attempts", CONFIG_SAVE_MAX_RETRIES);
    return false;
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
        ESP_LOGW(TAG, "New random credentials will be generated on next boot");
    }
}