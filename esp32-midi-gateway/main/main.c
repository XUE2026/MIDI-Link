#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "midi_engine.h"
#include "network_manager.h"
#include "udp_transport.h"
#include "web_server.h"
#include "ssh_service.h"
#include "tcp_control.h"
#include "ota_manager.h"
#include "reset_manager.h"
#include "auth_manager.h"
#include "config_manager.h"

static const char *TAG = "MIDI_GATEWAY";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-S3 Wireless MIDI Gateway starting...");

    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化配置管理器
    config_manager_init();

    // 初始化鉴权管理器
    auth_manager_init();

    // 初始化网络（默认SoftAP模式 192.168.3.1）
    network_manager_init();

    // 初始化Web服务器（端口8088）
    web_server_init();

    // 初始化SSH服务（端口22）
    ssh_service_init();

    // 初始化TCP控制通道（端口32）
    tcp_control_init();

    // 初始化OTA管理器
    ota_manager_init();

    // 初始化MIDI引擎（USB + BLE双模）
    midi_engine_init();

    // 初始化UDP传输层
    udp_transport_init();

    // 初始化重置管理器
    reset_manager_init();

    ESP_LOGI(TAG, "All subsystems initialized. System ready.");
}