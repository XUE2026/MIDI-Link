#include "tcp_control.h"
#include "config_manager.h"
#include "network_manager.h"
#include "midi_engine.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "TCP_CTRL";

static int server_fd = -1;

// 处理单个客户端连接
static void handle_client(int client_fd)
{
    char buf[TCP_CMD_BUF_SIZE];
    char resp[512];
    const char *prompt = "MIDI-GW> ";
    
    write(client_fd, "ESP32-S3 Wireless MIDI Gateway Console\r\n", 41);
    write(client_fd, "Commands: help, status, scan, list, brake, resume <pwd>, reset <pwd>, exit\r\n", 80);
    write(client_fd, prompt, strlen(prompt));
    
    while (1) {
        int len = read(client_fd, buf, sizeof(buf) - 1);
        if (len <= 0) break;
        buf[len] = 0;
        
        // 去除换行
        char *nl = strchr(buf, '\n');
        if (nl) *nl = 0;
        nl = strchr(buf, '\r');
        if (nl) *nl = 0;
        
        if (strcmp(buf, "help") == 0) {
            snprintf(resp, sizeof(resp),
                "Available commands:\r\n"
                "  help          - Show this help\r\n"
                "  status        - Show device status\r\n"
                "  scan          - Scan BLE MIDI devices\r\n"
                "  list          - List connected stations\r\n"
                "  brake         - Emergency stop MIDI forwarding\r\n"
                "  resume <pwd>  - Resume MIDI forwarding with sensitive password\r\n"
                "  reset <pwd>   - Factory reset with sensitive password\r\n"
                "  exit          - Disconnect\r\n");
            write(client_fd, resp, strlen(resp));
        } else if (strcmp(buf, "status") == 0) {
            snprintf(resp, sizeof(resp),
                "Network Mode: %s\r\n"
                "IP: %s\r\n"
                "USB MIDI: %s\r\n"
                "BLE MIDI: %s\r\n",
                network_manager_get_mode() == WIFI_MODE_AP ? "SoftAP" : "Station",
                network_manager_get_ip(),
                midi_engine_usb_is_connected() ? "Connected" : "Disconnected",
                midi_engine_ble_is_connected() ? "Connected" : "Disconnected"
            );
            write(client_fd, resp, strlen(resp));
        } else if (strcmp(buf, "scan") == 0) {
            midi_engine_ble_scan();
            write(client_fd, "BLE scan initiated\r\n", 20);
        } else if (strcmp(buf, "list") == 0) {
            char sta_list[8][20];
            int count = network_manager_get_sta_list(sta_list, 8);
            snprintf(resp, sizeof(resp), "Connected stations: %d\r\n", count);
            write(client_fd, resp, strlen(resp));
            for (int i = 0; i < count; i++) {
                snprintf(resp, sizeof(resp), "  %d. %s\r\n", i + 1, sta_list[i]);
                write(client_fd, resp, strlen(resp));
            }
        } else if (strcmp(buf, "brake") == 0) {
            midi_engine_emergency_stop();
            write(client_fd, "Emergency brake engaged\r\n", 25);
        } else if (strncmp(buf, "resume ", 7) == 0) {
            gateway_config_t config;
            config_manager_load(&config);
            if (strcmp(buf + 7, config.sensitive_password) == 0) {
                midi_engine_emergency_resume();
                write(client_fd, "Emergency brake released\r\n", 27);
            } else {
                write(client_fd, "Invalid password\r\n", 18);
            }
        } else if (strncmp(buf, "reset ", 6) == 0) {
            gateway_config_t config;
            config_manager_load(&config);
            if (strcmp(buf + 6, config.sensitive_password) == 0) {
                write(client_fd, "Factory reset initiated. Device will restart...\r\n", 50);
                config_manager_factory_reset();
                esp_restart();
            } else {
                write(client_fd, "Invalid password\r\n", 18);
            }
        } else if (strcmp(buf, "exit") == 0) {
            write(client_fd, "Goodbye\r\n", 9);
            break;
        } else if (strlen(buf) > 0) {
            write(client_fd, "Unknown command. Type 'help' for available commands.\r\n", 55);
        }
        
        write(client_fd, prompt, strlen(prompt));
    }
    
    close(client_fd);
}

// TCP服务器任务
static void tcp_server_task(void *pvParameters)
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        ESP_LOGE(TAG, "Failed to create TCP socket");
        vTaskDelete(NULL);
        return;
    }
    
    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(TCP_CONTROL_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind TCP port %d", TCP_CONTROL_PORT);
        close(server_fd);
        vTaskDelete(NULL);
        return;
    }
    
    listen(server_fd, TCP_MAX_CLIENTS);
    ESP_LOGI(TAG, "TCP control channel listening on port %d", TCP_CONTROL_PORT);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) continue;
        
        ESP_LOGI(TAG, "TCP client connected");
        handle_client(client_fd);
    }
}

void tcp_control_init(void)
{
    xTaskCreate(tcp_server_task, "tcp_control", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "TCP Control Channel task created (port %d)", TCP_CONTROL_PORT);
}