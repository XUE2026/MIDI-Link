#ifndef UDP_TRANSPORT_H
#define UDP_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "config_manager.h"

// OSC-like MIDI数据包格式
typedef struct __attribute__((packed)) {
    uint32_t timestamp;     // 时间戳（毫秒）
    uint8_t status;         // MIDI状态字节
    uint8_t data1;          // MIDI数据1
    uint8_t data2;          // MIDI数据2
    uint8_t channel;        // MIDI通道
} midi_packet_t;

#define MIDI_PACKET_SIZE sizeof(midi_packet_t)

// 初始化UDP传输层
void udp_transport_init(void);

// 设置目标（单播或广播）
void udp_transport_set_target(midi_output_target_t target, const char *ip, uint16_t port);

// 发送MIDI事件
void udp_transport_send(const midi_packet_t *packet);

// 启动/停止广播
void udp_transport_start_broadcast(void);
void udp_transport_stop_broadcast(void);

// 检查传输状态
bool udp_transport_is_active(void);

#endif // UDP_TRANSPORT_H