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
)