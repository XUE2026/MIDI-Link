#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "config_manager.h"

#define AUTH_TOKEN_LEN 64
#define AUTH_MAX_TOKENS 8
#define AUTH_TOKEN_EXPIRE_MS (30 * 60 * 1000)

typedef struct {
    char token[AUTH_TOKEN_LEN];
    uint32_t created_ms;
    bool active;
} auth_token_entry_t;

// 初始化鉴权管理器
void auth_manager_init(void);

// 校验接收端IP是否在白名单中（支持逗号分隔的多IP和CIDR格式）
bool auth_manager_check_ip(const char *ip);

// 生成新的认证Token
bool auth_manager_generate_token(char *token_out, size_t token_out_len);

// 验证Token是否有效
bool auth_manager_validate_token(const char *token);

// 注销Token
bool auth_manager_revoke_token(const char *token);

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