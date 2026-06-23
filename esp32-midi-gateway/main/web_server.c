#include "web_server.h"
#include "config_manager.h"
#include "network_manager.h"
#include "midi_engine.h"
#include "auth_manager.h"
#include "udp_transport.h"
#include "ota_manager.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "WEB_SRV";
static httpd_handle_t server = NULL;
static bool logged_in = false;

// --- 辅助函数 ---

static bool __attribute__((unused)) validate_sensitive_pwd(const char *input)
{
    gateway_config_t config;
    config_manager_load(&config);
    return (strcmp(input, config.sensitive_password) == 0);
}

// --- HTTP处理器 ---

static esp_err_t root_handler(httpd_req_t *req)
{
    extern const char index_html_start[] asm("_binary_index_html_start");
    extern const char index_html_end[] asm("_binary_index_html_end");
    const size_t index_html_size = index_html_end - index_html_start;
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html_start, index_html_size);
    return ESP_OK;
}

static esp_err_t style_handler(httpd_req_t *req)
{
    extern const char style_css_start[] asm("_binary_style_css_start");
    extern const char style_css_end[] asm("_binary_style_css_end");
    const size_t size = style_css_end - style_css_start;
    
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, style_css_start, size);
    return ESP_OK;
}

static esp_err_t script_handler(httpd_req_t *req)
{
    extern const char script_js_start[] asm("_binary_script_js_start");
    extern const char script_js_end[] asm("_binary_script_js_end");
    const size_t size = script_js_end - script_js_start;
    
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, script_js_start, size);
    return ESP_OK;
}

static esp_err_t login_handler(httpd_req_t *req)
{
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = 0;
    
    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *user = cJSON_GetObjectItem(json, "username");
    cJSON *pass = cJSON_GetObjectItem(json, "password");
    
    bool auth_ok = false;
    if (user && pass && 
        strcmp(user->valuestring, "xueyixuan2026") == 0 &&
        strcmp(pass->valuestring, "xueyixuan2026") == 0) {
        logged_in = true;
        auth_ok = true;
    }
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", auth_ok);
    
    const char *resp_str = cJSON_Print(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    cJSON_free((void *)resp_str);
    cJSON_Delete(json);
    cJSON_Delete(resp);
    return ESP_OK;
}

static esp_err_t status_handler(httpd_req_t *req)
{
    if (!logged_in) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Not logged in");
        return ESP_FAIL;
    }
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "ip", network_manager_get_ip());
    cJSON_AddStringToObject(json, "mode", 
        network_manager_get_mode() == WIFI_MODE_AP ? "SoftAP" : "Station");
    cJSON_AddStringToObject(json, "midi_source", 
        midi_engine_get_input_source() == MIDI_INPUT_USB ? "USB" :
        midi_engine_get_input_source() == MIDI_INPUT_BLE ? "BLE" : "Auto");
    cJSON_AddBoolToObject(json, "usb_connected", midi_engine_usb_is_connected());
    cJSON_AddBoolToObject(json, "ble_connected", midi_engine_ble_is_connected());
    
    const char *resp_str = cJSON_Print(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    cJSON_free((void *)resp_str);
    cJSON_Delete(json);
    return ESP_OK;
}

static esp_err_t config_get_handler(httpd_req_t *req)
{
    if (!logged_in) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Not logged in");
        return ESP_FAIL;
    }
    
    gateway_config_t config;
    config_manager_load(&config);
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "wifi_ssid", config.wifi_ssid);
    cJSON_AddStringToObject(json, "wifi_password", "******");
    cJSON_AddBoolToObject(json, "softap_mode", config.softap_mode);
    cJSON_AddNumberToObject(json, "midi_input", config.midi_input);
    cJSON_AddNumberToObject(json, "midi_output", config.midi_output);
    cJSON_AddStringToObject(json, "udp_target_ip", config.udp_target_ip);
    cJSON_AddNumberToObject(json, "udp_port", config.udp_port);
    cJSON_AddNumberToObject(json, "encryption", config.encryption);
    cJSON_AddNumberToObject(json, "auth_mode", config.auth_mode);
    cJSON_AddStringToObject(json, "ip_whitelist", config.ip_whitelist);
    
    const char *resp_str = cJSON_Print(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    cJSON_free((void *)resp_str);
    cJSON_Delete(json);
    return ESP_OK;
}

static esp_err_t config_update_handler(httpd_req_t *req)
{
    if (!logged_in) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Not logged in");
        return ESP_FAIL;
    }
    
    char buf[1024];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = 0;
    
    cJSON *json = cJSON_Parse(buf);
    if (!json) return ESP_FAIL;
    
    gateway_config_t config;
    config_manager_load(&config);
    
    cJSON *item;
    if ((item = cJSON_GetObjectItem(json, "wifi_ssid")) && item->valuestring)
        strncpy(config.wifi_ssid, item->valuestring, sizeof(config.wifi_ssid) - 1);
    if ((item = cJSON_GetObjectItem(json, "wifi_password")) && item->valuestring && 
        strcmp(item->valuestring, "******") != 0)
        strncpy(config.wifi_password, item->valuestring, sizeof(config.wifi_password) - 1);
    if ((item = cJSON_GetObjectItem(json, "softap_mode")))
        config.softap_mode = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(json, "midi_input")))
        config.midi_input = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "midi_output")))
        config.midi_output = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "udp_target_ip")) && item->valuestring)
        strncpy(config.udp_target_ip, item->valuestring, sizeof(config.udp_target_ip) - 1);
    if ((item = cJSON_GetObjectItem(json, "udp_port")))
        config.udp_port = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "encryption")))
        config.encryption = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "auth_mode")))
        config.auth_mode = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "ip_whitelist")) && item->valuestring)
        strncpy(config.ip_whitelist, item->valuestring, sizeof(config.ip_whitelist) - 1);
    
    config_manager_save(&config);
    
    if (cJSON_GetObjectItem(json, "wifi_ssid") || cJSON_GetObjectItem(json, "softap_mode")) {
        if (config.softap_mode) {
            network_manager_start_softap(config.wifi_ssid, config.wifi_password);
        } else {
            network_manager_start_station(config.wifi_ssid, config.wifi_password);
        }
    }
    
    udp_transport_set_target(config.midi_output, config.udp_target_ip, config.udp_port);
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    
    const char *resp_str = cJSON_Print(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    cJSON_free((void *)resp_str);
    cJSON_Delete(json);
    cJSON_Delete(resp);
    return ESP_OK;
}

static esp_err_t change_ssh_pwd_handler(httpd_req_t *req)
{
    if (!logged_in) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Not logged in");
        return ESP_FAIL;
    }
    
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = 0;
    
    cJSON *json = cJSON_Parse(buf);
    if (!json) return ESP_FAIL;
    
    cJSON *old_pwd = cJSON_GetObjectItem(json, "old_password");
    cJSON *new_pwd = cJSON_GetObjectItem(json, "new_password");
    cJSON *sensitive = cJSON_GetObjectItem(json, "sensitive_password");
    
    gateway_config_t config;
    config_manager_load(&config);
    
    bool ok = false;
    if (old_pwd && new_pwd && sensitive &&
        strcmp(sensitive->valuestring, config.sensitive_password) == 0 &&
        strcmp(old_pwd->valuestring, config.ssh_password) == 0) {
        strncpy(config.ssh_password, new_pwd->valuestring, sizeof(config.ssh_password) - 1);
        config_manager_save(&config);
        ok = true;
        ESP_LOGW(TAG, "SSH password changed!");
    }
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", ok);
    
    const char *resp_str = cJSON_Print(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    cJSON_free((void *)resp_str);
    cJSON_Delete(json);
    cJSON_Delete(resp);
    return ESP_OK;
}

static esp_err_t reset_device_handler(httpd_req_t *req)
{
    if (!logged_in) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Not logged in");
        return ESP_FAIL;
    }
    
    char buf[512];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = 0;
    
    cJSON *json = cJSON_Parse(buf);
    if (!json) return ESP_FAIL;
    
    cJSON *confirm_text = cJSON_GetObjectItem(json, "confirm_text");
    cJSON *reset_pwd = cJSON_GetObjectItem(json, "reset_password");
    cJSON *sensitive_pwd = cJSON_GetObjectItem(json, "sensitive_password");
    
    gateway_config_t config;
    config_manager_load(&config);
    
    bool ok = false;
    if (confirm_text && reset_pwd && sensitive_pwd &&
        strcmp(confirm_text->valuestring, "我已知晓执行此命令的后果，仍然继续") == 0 &&
        strcmp(reset_pwd->valuestring, "XUE2026") == 0 &&
        strcmp(sensitive_pwd->valuestring, config.sensitive_password) == 0) {
        ok = true;
    }
    
    if (ok) {
        config_manager_factory_reset();
        esp_restart();
    }
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", ok);
    
    const char *resp_str = cJSON_Print(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    cJSON_free((void *)resp_str);
    cJSON_Delete(json);
    cJSON_Delete(resp);
    return ESP_OK;
}

static esp_err_t ble_scan_handler(httpd_req_t *req)
{
    if (!logged_in) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Not logged in");
        return ESP_FAIL;
    }
    
    midi_engine_ble_scan();
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    
    const char *resp_str = cJSON_Print(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    cJSON_free((void *)resp_str);
    cJSON_Delete(resp);
    return ESP_OK;
}

static esp_err_t emergency_brake_handler(httpd_req_t *req)
{
    if (!logged_in) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Not logged in");
        return ESP_FAIL;
    }
    
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = 0;
    
    cJSON *json = cJSON_Parse(buf);
    if (!json) return ESP_FAIL;
    
    cJSON *action = cJSON_GetObjectItem(json, "action");
    cJSON *password = cJSON_GetObjectItem(json, "password");
    
    bool success = false;
    if (action && strcmp(action->valuestring, "engage") == 0) {
        midi_engine_emergency_stop();
        success = true;
    } else if (action && password && strcmp(action->valuestring, "release") == 0) {
        gateway_config_t config;
        config_manager_load(&config);
        if (strcmp(password->valuestring, config.sensitive_password) == 0) {
            midi_engine_emergency_resume();
            success = true;
        }
    }
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", success);
    
    const char *resp_str = cJSON_Print(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    cJSON_free((void *)resp_str);
    cJSON_Delete(json);
    cJSON_Delete(resp);
    return ESP_OK;
}

static esp_err_t ota_upload_handler(httpd_req_t *req)
{
    if (!logged_in) {
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Not logged in");
        return ESP_FAIL;
    }
    
    size_t total_len = req->content_len;
    bool backup = false;
    
    uint8_t *fw_data = malloc(total_len);
    if (!fw_data) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }
    
    size_t received = 0;
    while (received < total_len) {
        int ret = httpd_req_recv(req, (char *)fw_data + received, 
                                  (total_len - received) > 1024 ? 1024 : (total_len - received));
        if (ret <= 0) {
            free(fw_data);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
            return ESP_FAIL;
        }
        received += ret;
    }
    
    char qs[64];
    char backup_str[8];
    if (httpd_req_get_url_query_str(req, qs, sizeof(qs)) == ESP_OK &&
        httpd_query_key_value(qs, "backup", backup_str, sizeof(backup_str)) == ESP_OK) {
        backup = (strcmp(backup_str, "1") == 0);
    }
    
    if (backup) {
        ota_manager_create_checkpoint();
    }
    
    bool success = ota_manager_update(fw_data, total_len);
    free(fw_data);
    
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", success);
    if (!success) {
        cJSON_AddStringToObject(resp, "error", "OTA update failed");
    }
    
    const char *resp_str = cJSON_Print(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    cJSON_free((void *)resp_str);
    cJSON_Delete(resp);
    return ESP_OK;
}

void web_server_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEB_SERVER_PORT;
    config.max_uri_handlers = 16;
    
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server on port %d", WEB_SERVER_PORT);
        return;
    }
    
    httpd_uri_t uri_root = { .uri = "/", .method = HTTP_GET, .handler = root_handler };
    httpd_register_uri_handler(server, &uri_root);
    
    httpd_uri_t uri_style = { .uri = "/style.css", .method = HTTP_GET, .handler = style_handler };
    httpd_register_uri_handler(server, &uri_style);
    
    httpd_uri_t uri_script = { .uri = "/script.js", .method = HTTP_GET, .handler = script_handler };
    httpd_register_uri_handler(server, &uri_script);
    
    httpd_uri_t uri_login = { .uri = "/api/login", .method = HTTP_POST, .handler = login_handler };
    httpd_register_uri_handler(server, &uri_login);
    
    httpd_uri_t uri_status = { .uri = "/api/status", .method = HTTP_GET, .handler = status_handler };
    httpd_register_uri_handler(server, &uri_status);
    
    httpd_uri_t uri_config_get = { .uri = "/api/config", .method = HTTP_GET, .handler = config_get_handler };
    httpd_register_uri_handler(server, &uri_config_get);
    
    httpd_uri_t uri_config_update = { .uri = "/api/config", .method = HTTP_POST, .handler = config_update_handler };
    httpd_register_uri_handler(server, &uri_config_update);
    
    httpd_uri_t uri_ssh_pwd = { .uri = "/api/change-ssh-password", .method = HTTP_POST, .handler = change_ssh_pwd_handler };
    httpd_register_uri_handler(server, &uri_ssh_pwd);
    
    httpd_uri_t uri_reset = { .uri = "/api/reset-device", .method = HTTP_POST, .handler = reset_device_handler };
    httpd_register_uri_handler(server, &uri_reset);
    
    httpd_uri_t uri_ble_scan = { .uri = "/api/ble-scan", .method = HTTP_POST, .handler = ble_scan_handler };
    httpd_register_uri_handler(server, &uri_ble_scan);
    
    httpd_uri_t uri_brake = { .uri = "/api/emergency-brake", .method = HTTP_POST, .handler = emergency_brake_handler };
    httpd_register_uri_handler(server, &uri_brake);
    
    httpd_uri_t uri_ota = { .uri = "/api/ota-upload", .method = HTTP_POST, .handler = ota_upload_handler };
    httpd_register_uri_handler(server, &uri_ota);
    
    ESP_LOGI(TAG, "Web server started on port %d", WEB_SERVER_PORT);
}

bool web_server_authenticate(const char *username, const char *password)
{
    return (strcmp(username, "xueyixuan2026") == 0 && 
            strcmp(password, "xueyixuan2026") == 0);
}

bool web_server_is_logged_in(void)
{
    return logged_in;
}