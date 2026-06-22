#include "midi_engine.h"
#include "udp_transport.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "MIDI_ENGINE";

static midi_input_source_t current_input = MIDI_INPUT_AUTO;
static midi_event_callback_t user_callback = NULL;
static void *callback_user_data = NULL;
static bool emergency_stopped = false;

// USB状态
static bool usb_connected = false;

// BLE状态
static bool ble_connected = false;

// MIDI事件转UDP数据包回调
static void midi_to_udp_callback(const midi_event_t *event, void *user_data)
{
    midi_packet_t packet;
    packet.timestamp = (uint32_t)(esp_timer_get_time() / 1000);
    packet.status = event->status;
    packet.data1 = event->data1;
    packet.data2 = event->data2;
    packet.channel = event->channel;
    
    udp_transport_send(&packet);
}

// 内部调用：分发MIDI事件
static void midi_engine_dispatch(const midi_event_t *event)
{
    if (emergency_stopped) {
        return;
    }
    
    if (user_callback) {
        user_callback(event, callback_user_data);
    }
}

void midi_engine_init(void)
{
    ESP_LOGI(TAG, "Initializing MIDI Engine");
    current_input = MIDI_INPUT_AUTO;
    emergency_stopped = false;
    
    // USB Host初始化
    midi_engine_usb_init();
    
    // BLE初始化
    midi_engine_ble_init();
    
    // 注册UDP转发回调
    midi_engine_register_callback(midi_to_udp_callback, NULL);
    
    ESP_LOGI(TAG, "MIDI Engine initialized with auto UDP forwarding");
}

void midi_engine_set_input_source(midi_input_source_t source)
{
    current_input = source;
    ESP_LOGI(TAG, "MIDI input source set to %d", source);
}

midi_input_source_t midi_engine_get_input_source(void)
{
    return current_input;
}

void midi_engine_register_callback(midi_event_callback_t cb, void *user_data)
{
    user_callback = cb;
    callback_user_data = user_data;
}

void midi_engine_usb_init(void)
{
    ESP_LOGI(TAG, "USB Host MIDI initialized");
}

bool midi_engine_usb_is_connected(void)
{
    return usb_connected;
}

void midi_engine_ble_init(void)
{
    ESP_LOGI(TAG, "BLE MIDI initialized");
}

void midi_engine_ble_scan(void)
{
    ESP_LOGI(TAG, "BLE scanning...");
}

bool midi_engine_ble_connect(const uint8_t *addr, const char *name)
{
    ESP_LOGI(TAG, "BLE connecting to %s", name ? name : "unknown");
    ble_connected = true;
    return true;
}

void midi_engine_ble_disconnect(void)
{
    ble_connected = false;
    ESP_LOGI(TAG, "BLE disconnected");
}

bool midi_engine_ble_is_connected(void)
{
    return ble_connected;
}

void midi_engine_emergency_stop(void)
{
    emergency_stopped = true;
    ESP_LOGW(TAG, "EMERGENCY BRAKE ACTIVE - MIDI forwarding stopped");
}

void midi_engine_emergency_resume(void)
{
    emergency_stopped = false;
    ESP_LOGI(TAG, "Emergency brake released - MIDI forwarding resumed");
}