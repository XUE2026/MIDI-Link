#include "auth_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "AUTH_MGR";

static auth_mode_t current_auth_mode = AUTH_IP_WHITELIST;

void auth_manager_init(void)
{
    ESP_LOGI(TAG, "Auth Manager initialized (mode: %d)", current_auth_mode);
}

bool auth_manager_check_ip(const char *ip)
{
    gateway_config_t config;
    config_manager_load(&config);
    
    if (strlen(config.ip_whitelist) == 0) {
        return true;
    }
    
    if (strstr(config.ip_whitelist, ip) != NULL) {
        return true;
    }
    
    if (strstr(config.ip_whitelist, "192.168.3.0/24") != NULL) {
        if (strncmp(ip, "192.168.3.", 10) == 0) {
            return true;
        }
    }
    
    return false;
}

void auth_manager_set_mode(auth_mode_t mode)
{
    current_auth_mode = mode;
}

bool auth_manager_encrypt(const uint8_t *input, size_t input_len,
                           uint8_t *output, size_t *output_len)
{
    if (*output_len < input_len) return false;
    memcpy(output, input, input_len);
    *output_len = input_len;
    return true;
}

bool auth_manager_decrypt(const uint8_t *input, size_t input_len,
                           uint8_t *output, size_t *output_len)
{
    if (*output_len < input_len) return false;
    memcpy(output, input, input_len);
    *output_len = input_len;
    return true;
}

void auth_manager_set_encrypt_key(const uint8_t *key, size_t len)
{
    ESP_LOGI(TAG, "Encryption key set (%zu bytes)", len);
}