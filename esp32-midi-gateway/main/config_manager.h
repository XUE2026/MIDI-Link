#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_NAMESPACE "midi_gw"
#define MIDI_UDP_PORT_DEFAULT 5000

typedef enum {
    MIDI_INPUT_USB = 0,
    MIDI_INPUT_BLE = 1,
    MIDI_INPUT_AUTO = 2,
} midi_input_source_t;

typedef enum {
    MIDI_OUTPUT_UNICAST = 0,
    MIDI_OUTPUT_BROADCAST = 1,
} midi_output_target_t;

typedef enum {
    ENCRYPT_NONE = 0,
    ENCRYPT_AES128 = 1,
} encryption_mode_t;

typedef enum {
    AUTH_IP_WHITELIST = 0,
    AUTH_KEY = 1,
} auth_mode_t;

typedef struct {
    // Wi-Fi配置
    char wifi_ssid[32];
    char wifi_password[64];
    bool softap_mode;           // true=SoftAP, false=Station
    
    // MIDI配置
    midi_input_source_t midi_input;
    midi_output_target_t midi_output;
    char udp_target_ip[16];
    uint16_t udp_port;
    
    // 安全配置
    encryption_mode_t encryption;
    auth_mode_t auth_mode;
    char auth_key[32];
    char ip_whitelist[256];     // 逗号分隔IP列表
    char ssh_password[64];
    char sensitive_password[64];
    
    // 蓝牙配置
    char ble_device_name[64];
    uint8_t ble_device_addr[6];
    bool ble_paired;
    
    // 状态
    bool emergency_brake;
} gateway_config_t;

// 初始化配置管理器
void config_manager_init(void);

// 加载/保存配置
void config_manager_load(gateway_config_t *config);
void config_manager_save(const gateway_config_t *config);

// 获取默认配置
void config_manager_get_default(gateway_config_t *config);

// 重置为出厂设置
void config_manager_factory_reset(void);

#endif // CONFIG_MANAGER_H