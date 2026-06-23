#include "auth_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "AUTH_MGR";

static auth_mode_t current_auth_mode = AUTH_IP_WHITELIST;
static auth_token_entry_t tokens[AUTH_MAX_TOKENS];
static uint8_t encrypt_key[16];
static bool encrypt_key_set = false;

static uint32_t get_timestamp_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static bool ip_in_cidr(const char *ip, const char *cidr)
{
    uint32_t ip_addr = 0;
    uint32_t net_addr = 0;
    uint32_t mask = 0;
    int prefix = 32;
    char cidr_copy[32];
    char *slash;

    if (strlen(cidr) >= sizeof(cidr_copy)) {
        return false;
    }
    strncpy(cidr_copy, cidr, sizeof(cidr_copy) - 1);
    cidr_copy[sizeof(cidr_copy) - 1] = '\0';

    slash = strchr(cidr_copy, '/');
    if (slash) {
        *slash = '\0';
        prefix = atoi(slash + 1);
        if (prefix < 0 || prefix > 32) {
            return false;
        }
    }

    unsigned int ip_bytes[4];
    unsigned int net_bytes[4];
    if (sscanf(ip, "%u.%u.%u.%u", &ip_bytes[0], &ip_bytes[1], &ip_bytes[2], &ip_bytes[3]) != 4) {
        return false;
    }
    if (sscanf(cidr_copy, "%u.%u.%u.%u", &net_bytes[0], &net_bytes[1], &net_bytes[2], &net_bytes[3]) != 4) {
        return false;
    }

    ip_addr = (ip_bytes[0] << 24) | (ip_bytes[1] << 16) | (ip_bytes[2] << 8) | ip_bytes[3];
    net_addr = (net_bytes[0] << 24) | (net_bytes[1] << 16) | (net_bytes[2] << 8) | net_bytes[3];

    if (prefix == 0) {
        mask = 0;
    } else {
        mask = 0xFFFFFFFF << (32 - prefix);
    }

    return (ip_addr & mask) == (net_addr & mask);
}

void auth_manager_init(void)
{
    memset(tokens, 0, sizeof(tokens));
    ESP_LOGI(TAG, "Auth Manager initialized (mode: %d)", current_auth_mode);
}

bool auth_manager_check_ip(const char *ip)
{
    gateway_config_t config;
    config_manager_load(&config);

    if (strlen(config.ip_whitelist) == 0) {
        return true;
    }

    char whitelist_copy[256];
    strncpy(whitelist_copy, config.ip_whitelist, sizeof(whitelist_copy) - 1);
    whitelist_copy[sizeof(whitelist_copy) - 1] = '\0';

    char *token = strtok(whitelist_copy, ",");
    while (token != NULL) {
        while (*token == ' ') token++;

        if (strstr(token, "/") != NULL) {
            if (ip_in_cidr(ip, token)) {
                return true;
            }
        } else {
            if (strncmp(ip, token, strlen(token)) == 0) {
                return true;
            }
        }
        token = strtok(NULL, ",");
    }

    return false;
}

bool auth_manager_generate_token(char *token_out, size_t token_out_len)
{
    if (token_out == NULL || token_out_len < AUTH_TOKEN_LEN) {
        return false;
    }

    int slot = -1;
    uint32_t now = get_timestamp_ms();
    for (int i = 0; i < AUTH_MAX_TOKENS; i++) {
        if (!tokens[i].active || (now - tokens[i].created_ms > AUTH_TOKEN_EXPIRE_MS)) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        ESP_LOGW(TAG, "No available token slots");
        return false;
    }

    uint8_t random_bytes[32];
    esp_fill_random(random_bytes, sizeof(random_bytes));

    char token_str[AUTH_TOKEN_LEN];
    int pos = 0;
    for (int i = 0; i < (int)sizeof(random_bytes) && pos < AUTH_TOKEN_LEN - 1; i++) {
        pos += snprintf(token_str + pos, AUTH_TOKEN_LEN - pos, "%02x", random_bytes[i]);
    }
    token_str[AUTH_TOKEN_LEN - 1] = '\0';

    strncpy(tokens[slot].token, token_str, AUTH_TOKEN_LEN - 1);
    tokens[slot].token[AUTH_TOKEN_LEN - 1] = '\0';
    tokens[slot].created_ms = now;
    tokens[slot].active = true;

    strncpy(token_out, token_str, token_out_len - 1);
    token_out[token_out_len - 1] = '\0';

    ESP_LOGI(TAG, "Token generated (slot %d)", slot);
    return true;
}

bool auth_manager_validate_token(const char *token)
{
    if (token == NULL || strlen(token) == 0) {
        return false;
    }

    uint32_t now = get_timestamp_ms();
    for (int i = 0; i < AUTH_MAX_TOKENS; i++) {
        if (tokens[i].active &&
            strncmp(tokens[i].token, token, AUTH_TOKEN_LEN) == 0) {
            if (now - tokens[i].created_ms <= AUTH_TOKEN_EXPIRE_MS) {
                return true;
            } else {
                tokens[i].active = false;
                ESP_LOGI(TAG, "Token %d expired", i);
                return false;
            }
        }
    }
    return false;
}

bool auth_manager_revoke_token(const char *token)
{
    if (token == NULL) {
        return false;
    }

    for (int i = 0; i < AUTH_MAX_TOKENS; i++) {
        if (tokens[i].active &&
            strncmp(tokens[i].token, token, AUTH_TOKEN_LEN) == 0) {
            tokens[i].active = false;
            ESP_LOGI(TAG, "Token %d revoked", i);
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
    if (key && len > 0) {
        size_t copy_len = (len < sizeof(encrypt_key)) ? len : sizeof(encrypt_key);
        memcpy(encrypt_key, key, copy_len);
        encrypt_key_set = true;
        ESP_LOGI(TAG, "Encryption key set (%zu bytes)", copy_len);
    }
}
