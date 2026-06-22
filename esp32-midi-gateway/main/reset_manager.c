#include "reset_manager.h"
#include "config_manager.h"
#include "esp_log.h"
#include "esp_system.h"
#include <string.h>

static const char *TAG = "RESET_MGR";

static const char *REQUIRED_CONFIRM_TEXT = "我已知晓执行此命令的后果，仍然继续";
static const char *REQUIRED_RESET_PWD = "XUE2026";

void reset_manager_init(void)
{
    ESP_LOGI(TAG, "Reset Manager initialized");
    ESP_LOGI(TAG, "Factory reset requires 3-step confirmation");
}

bool reset_manager_execute(const char *confirm_text,
                            const char *reset_password,
                            const char *sensitive_password)
{
    if (!confirm_text || strcmp(confirm_text, REQUIRED_CONFIRM_TEXT) != 0) {
        ESP_LOGW(TAG, "Factory reset denied: incorrect confirm text");
        return false;
    }
    
    if (!reset_password || strcmp(reset_password, REQUIRED_RESET_PWD) != 0) {
        ESP_LOGW(TAG, "Factory reset denied: incorrect reset password");
        return false;
    }
    
    gateway_config_t config;
    config_manager_load(&config);
    if (!sensitive_password || strcmp(sensitive_password, config.sensitive_password) != 0) {
        ESP_LOGW(TAG, "Factory reset denied: incorrect sensitive password");
        return false;
    }
    
    ESP_LOGW(TAG, "ALL CONFIRMATION STEPS PASSED. EXECUTING FACTORY RESET!");
    config_manager_factory_reset();
    esp_restart();
    return true;
}

int reset_manager_get_step(void)
{
    return 0;
}