#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "config_manager.h"

// 初始化鉴权管理器
void auth_manager_init(void);

// 校验接收端IP是否在白名单中
bool auth_manager_check_ip(const char *ip);

// AES-128 加密/解密
bool auth_manager_encrypt(const uint8_t *input, size_t input_len,
                           uint8_t *output, size_t *output_len);
bool auth_manager_decrypt(const uint8_t *input, size_t input_len,
                           uint8_t *output, size_t *output_len);

// 设置加密密钥
void auth_manager_set_encrypt_key(const uint8_t *key, size_t len);

// 设置鉴权方式
void auth_manager_set_mode(auth_mode_t mode);

#endif // AUTH_MANAGER_H