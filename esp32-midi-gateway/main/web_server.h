#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdbool.h>

#define WEB_SERVER_PORT 8088

// 初始化Web服务器
void web_server_init(void);

// Web认证
bool web_server_authenticate(const char *username, const char *password);

// 获取Web登录状态
bool web_server_is_logged_in(void);

#endif // WEB_SERVER_H