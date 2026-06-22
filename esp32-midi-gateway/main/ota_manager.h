#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

// 初始化OTA管理器
void ota_manager_init(void);

// 创建冰点备份（将当前固件复制到OTA回滚分区）
bool ota_manager_create_checkpoint(void);

// 执行OTA升级（从接收到的固件数据）
bool ota_manager_update(const uint8_t *data, size_t len);

// 获取当前运行的分区标签
const char *ota_manager_get_running_partition(void);

// 检查是否需要回滚（看门狗检测启动失败）
void ota_manager_check_rollback(void);

#endif // OTA_MANAGER_H