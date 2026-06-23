#ifndef SSH_SERVICE_H
#define SSH_SERVICE_H

#include <stdbool.h>

#define SSH_PORT 22
#define SSH_USER "admin"

// SSH服务状态：占位实现，未实际启用
// 说明：ESP-IDF不提供原生SSH服务器组件。
// 生产部署需要集成 esp_ssh_cli_server 组件（基于tinySSH的移植）。
// 当前TCP控制通道（端口32）可作为替代方案。

void ssh_service_init(void);

// 检查SSH服务是否实际可用（当前始终返回false）
bool ssh_service_is_available(void);

#endif // SSH_SERVICE_H