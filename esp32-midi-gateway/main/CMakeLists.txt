idf_component_register(
    SRCS
        "main.c"
        "midi_engine.c"
        "network_manager.c"
        "udp_transport.c"
        "web_server.c"
        "ssh_service.c"
        "tcp_control.c"
        "ota_manager.c"
        "reset_manager.c"
        "auth_manager.c"
        "config_manager.c"
    INCLUDE_DIRS "."
    EMBED_FILES "web_assets/index.html"
    EMBED_FILES "web_assets/style.css"
    EMBED_FILES "web_assets/script.js"
    REQUIRES cjson esp_http_server nvs_flash esp_wifi esp_event esp_netif lwip esp_timer app_update mbedtls
)