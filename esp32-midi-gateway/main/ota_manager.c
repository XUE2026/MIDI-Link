#include "ota_manager.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_image_format.h"
#include <string.h>

static const char *TAG = "OTA_MGR";

void ota_manager_init(void)
{
    ESP_LOGI(TAG, "OTA Manager initialized");
    ESP_LOGI(TAG, "Running partition: %s", ota_manager_get_running_partition());
    ota_manager_check_rollback();
}

const char *ota_manager_get_running_partition(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running) {
        return running->label;
    }
    return "unknown";
}

bool ota_manager_create_checkpoint(void)
{
    ESP_LOGI(TAG, "Creating checkpoint (ice point)...");
    
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    
    if (!running || !next) {
        ESP_LOGE(TAG, "Failed to get partitions for checkpoint");
        return false;
    }
    
    esp_err_t err = esp_ota_set_boot_partition(running);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set boot partition for checkpoint: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "Checkpoint created - current firmware backed up");
    return true;
}

bool ota_manager_update(const uint8_t *data, size_t len)
{
    ESP_LOGI(TAG, "Starting OTA update (%zu bytes)...", len);
    
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        ESP_LOGE(TAG, "No OTA update partition available");
        return false;
    }
    
    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(update_partition, len, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        return false;
    }
    
    err = esp_ota_write(ota_handle, data, len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(err));
        esp_ota_abort(ota_handle);
        return false;
    }
    
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA end failed: %s", esp_err_to_name(err));
        return false;
    }
    
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGW(TAG, "OTA update complete. Rebooting to new partition...");
    esp_restart();
    return true;
}

void ota_manager_check_rollback(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *boot = esp_ota_get_boot_partition();
    
    if (running && boot && running != boot) {
        ESP_LOGW(TAG, "Boot partition != Running partition: %s vs %s",
                 boot->label, running->label);
    }
    
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
        if (state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "New firmware detected, marking as valid");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
}