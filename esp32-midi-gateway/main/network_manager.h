#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <stdbool.h>
#include "esp_wifi.h"

#define SOFTAP_IP "192.168.3.1"
#define SOFTAP_NETMASK "255.255.255.0"
#define SOFTAP_GATEWAY "192.168.3.1"

// 初始化网络管理器
void network_manager_init(void);

// 启动SoftAP模式
void network_manager_start_softap(const char *ssid, const char *password);

// 启动Station模式（连接路由器）
void network_manager_start_station(const char *ssid, const char *password);

// 切换模式
void network_manager_set_mode(wifi_mode_t mode);

// 获取当前模式
wifi_mode_t network_manager_get_mode(void);

// 获取本机IP
const char *network_manager_get_ip(void);

// 获取连接的Station设备列表（SoftAP模式下）
int network_manager_get_sta_list(char output[][16], int max_count);

#endif // NETWORK_MANAGER_H