#ifndef RESET_MANAGER_H
#define RESET_MANAGER_H

#include <stdbool.h>

// 初始化重置管理器
void reset_manager_init(void);

// 执行三重确认重置（由Web或TCP端发起）
bool reset_manager_execute(const char *confirm_text, 
                            const char *reset_password,
                            const char *sensitive_password);

// 获取当前重置状态
int reset_manager_get_step(void);

#endif // RESET_MANAGER_H