#include "udp_transport.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>
#include <sys/time.h>
#include <errno.h>

static const char *TAG = "UDP_XPORT";

#define UDP_SEND_MAX_RETRIES 3
#define UDP_SEND_RETRY_DELAY_MS 10

static int udp_socket = -1;
static struct sockaddr_in dest_addr;
static bool broadcast_mode = false;
static bool active = false;

void udp_transport_init(void)
{
    ESP_LOGI(TAG, "Initializing UDP Transport");
    
    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket < 0) {
        ESP_LOGE(TAG, "Failed to create UDP socket");
        return;
    }
    
    int broadcast = 1;
    setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(MIDI_UDP_PORT_DEFAULT);
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    
    active = true;
    ESP_LOGI(TAG, "UDP Transport initialized (port: %d)", MIDI_UDP_PORT_DEFAULT);
}

void udp_transport_set_target(midi_output_target_t target, const char *ip, uint16_t port)
{
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    
    if (target == MIDI_OUTPUT_BROADCAST) {
        dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        broadcast_mode = true;
        ESP_LOGI(TAG, "UDP target set to BROADCAST port %d", port);
    } else {
        inet_aton(ip, &dest_addr.sin_addr);
        broadcast_mode = false;
        ESP_LOGI(TAG, "UDP target set to UNICAST %s:%d", ip, port);
    }
}

bool udp_transport_send(const midi_packet_t *packet)
{
    if (!active || udp_socket < 0 || packet == NULL) {
        return false;
    }

    for (int attempt = 0; attempt < UDP_SEND_MAX_RETRIES; attempt++) {
        int ret = sendto(udp_socket, packet, MIDI_PACKET_SIZE, 0,
                         (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (ret >= 0) {
            return true;
        }

        ESP_LOGW(TAG, "UDP send attempt %d/%d failed: errno %d",
                 attempt + 1, UDP_SEND_MAX_RETRIES, errno);

        if (attempt < UDP_SEND_MAX_RETRIES - 1) {
            vTaskDelay(pdMS_TO_TICKS(UDP_SEND_RETRY_DELAY_MS));
        }
    }

    ESP_LOGE(TAG, "UDP send failed after %d attempts", UDP_SEND_MAX_RETRIES);
    return false;
}

void udp_transport_start_broadcast(void)
{
    broadcast_mode = true;
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    ESP_LOGI(TAG, "UDP broadcast started");
}

void udp_transport_stop_broadcast(void)
{
    broadcast_mode = false;
    ESP_LOGI(TAG, "UDP broadcast stopped");
}

bool udp_transport_is_active(void)
{
    return active;
}