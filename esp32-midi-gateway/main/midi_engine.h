#ifndef MIDI_ENGINE_H
#define MIDI_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "config_manager.h"

// MIDI消息结构
typedef struct __attribute__((packed)) {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    uint8_t channel;   // 0-15
} midi_event_t;

// MIDI引擎状态回调
typedef void (*midi_event_callback_t)(const midi_event_t *event, void *user_data);

// 初始化MIDI引擎
void midi_engine_init(void);

// 设置输入源
void midi_engine_set_input_source(midi_input_source_t source);

// 获取当前输入源
midi_input_source_t midi_engine_get_input_source(void);

// 注册MIDI事件回调
void midi_engine_register_callback(midi_event_callback_t cb, void *user_data);

// USB Host MIDI接口
void midi_engine_usb_init(void);
bool midi_engine_usb_is_connected(void);

// BLE MIDI接口
void midi_engine_ble_init(void);
void midi_engine_ble_scan(void);
bool midi_engine_ble_connect(const uint8_t *addr, const char *name);
void midi_engine_ble_disconnect(void);
bool midi_engine_ble_is_connected(void);

// 紧急停止MIDI转发
void midi_engine_emergency_stop(void);
void midi_engine_emergency_resume(void);

#endif // MIDI_ENGINE_H