#include "ssh_service.h"
#include "config_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SSH_SRV";

/*
 * ============================================================
 * SSH服务 - 占位实现说明
 * ============================================================
 *
 * 状态：未实现（仅接口占位）
 *
 * 原因：ESP-IDF不提供原生SSH服务器组件。
 *
 * 生产部署方案：
 *   需要集成 esp_ssh_cli_server 组件（基于tinySSH的移植）。
 *   该组件不在标准ESP-IDF中，需单独获取。
 *
 * 替代方案：
 *   TCP控制通道（端口32）当前可用，提供类似的命令行控制功能。
 *   详见 tcp_control.c / tcp_control.h
 *
 * 未来计划：
 *   当SSH组件集成完成后，取消注释下方代码并删除此说明。
 * ============================================================
 */

void ssh_service_init(void)
{
    ESP_LOGI(TAG, "SSH Service initializing on port %d...", SSH_PORT);
    ESP_LOGI(TAG, "SSH user: %s (password configurable via Web UI)", SSH_USER);

    /*
     * === 生产代码（待实现）===
     *
     * ssh_cli_server_config_t config = SSH_CLI_SERVER_CONFIG_DEFAULT();
     * config.port = SSH_PORT;
     * config.username = SSH_USER;
     *
     * gateway_config_t gw_config;
     * config_manager_load(&gw_config);
     * config.password = gw_config.ssh_password;
     *
     * esp_err_t ret = ssh_cli_server_start(&config);
     * if (ret == ESP_OK) {
     *     ESP_LOGI(TAG, "SSH server started successfully");
     * } else {
     *     ESP_LOGE(TAG, "SSH server start failed: %s", esp_err_to_name(ret));
     * }
     */

    ESP_LOGW(TAG, "========================================");
    ESP_LOGW(TAG, "  SSH SERVER: NOT IMPLEMENTED");
    ESP_LOGW(TAG, "  SSH server requires esp_ssh_cli_server component");
    ESP_LOGW(TAG, "  Use TCP control channel on port 32 instead");
    ESP_LOGW(TAG, "========================================");
}

bool ssh_service_is_available(void)
{
    return false;
}
