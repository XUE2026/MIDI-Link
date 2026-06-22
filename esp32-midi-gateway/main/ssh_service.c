#include "ssh_service.h"
#include "config_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SSH_SRV";

/*
 * SSH服务封装说明：
 * ESP-IDF不提供原生SSH服务器组件。
 * 生产部署需要集成 esp_ssh_cli_server 组件（基于tinySSH的移植）。
 * 以下为接口占位实现，实际编译时需包含该组件。
 */

void ssh_service_init(void)
{
    ESP_LOGI(TAG, "SSH Service initializing on port %d...", SSH_PORT);
    ESP_LOGI(TAG, "SSH user: %s (password changeable via Web UI)", SSH_USER);
    
    // 生产代码应调用：
    // ssh_cli_server_config_t config = SSH_CLI_SERVER_CONFIG_DEFAULT();
    // config.port = SSH_PORT;
    // config.username = SSH_USER;
    // gateway_config_t gw_config;
    // config_manager_load(&gw_config);
    // config.password = gw_config.ssh_password;
    // ssh_cli_server_start(&config);
    
    ESP_LOGW(TAG, "SSH server requires esp_ssh_cli_server component");
    ESP_LOGW(TAG, "TCP control channel on port 32 available as alternative");
}