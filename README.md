# MIDI-Link: ESP32-S3 无线MIDI网关

将电子琴/电钢琴通过 **USB-OTG** 或 **BLE-MIDI** 无线连接到手机上运行的 MuseScore，实现无线乐谱输入。ESP32-S3 作为硬件网关，接收 MIDI 数据并通过 Wi-Fi 局域网转发，手机端 Python 脚本接收后注入 ALSA 虚拟 MIDI 端口供 MuseScore 使用。

## 系统架构

```
┌──────────────────────┐       Wi-Fi UDP/OSC       ┌──────────────────────┐
│   ESP32-S3 N16R8     │ ◄──────────────────────► │   Android Termux     │
│                      │                           │   (proot-debian)     │
│  ┌──────┐  ┌──────┐  │                           │  ┌────────────────┐  │
│  │USB   │  │BLE   │  │                           │  │midi_receiver.py│  │
│  │Host  │──►MIDI  │──│  UDP/OSC  ──────────────► │  │(Python)        │──│─► ALSA snd-virmidi ──► MuseScore
│  │MIDI  │  │Engine│  │                           │  └────────────────┘  │
│  └──────┘  └──────┘  │                           └──────────────────────┘
│                       │
│  ┌──────────────────┐ │
│  │ Web Console      │ │   端口 8088 (配置管理)
│  │ SSH Service      │ │   端口 22  (远程登录)
│  │ TCP Control      │ │   端口 32  (控制通道)
│  │ OTA Manager      │ │   固件升级与回滚
│  └──────────────────┘ │
└──────────────────────┘
```

## 功能特性

### ESP32-S3 固件
- **双模 MIDI 输入**: USB Host MIDI (OTG) + BLE-MIDI Central 并行接收
- **UDP/OSC 转发**: 低延迟局域网无线传输
- **Wi-Fi 双模**: SoftAP (直连) + Station (路由器) 模式
- **Web 管理后台**: 端口 8088，网页配置网络、查看设备状态
- **SSH 服务**: 端口 22，远程命令行管理
- **TCP 控制通道**: 端口 32，程序化控制
- **OTA 升级**: 支持固件在线升级与冰点回滚
- **三重确认重置**: 长按重置按钮保护
- **配置持久化**: NVS 存储，断电不丢失

### 手机接收端 (Termux)
- **UDP 接收**: 监听端口接收 MIDI 事件
- **ALSA 虚拟 MIDI 注入**: 通过 `snd-virmidi` 内核模块
- **自动重连**: 网络中断自动恢复
- **Python 脚本**: 轻量无依赖，仅需 `python-rtmidi`

## 硬件要求

| 组件 | 规格 |
|------|------|
| **SoC** | ESP32-S3 (Xtensa LX7 双核 240MHz) |
| **Flash** | 16MB (Quad SPI, 80MHz) |
| **PSRAM** | 8MB (Octal SPI) |
| **USB** | USB-OTG 接口连接电子琴 |
| **BLE** | 内置 BLE 5.0 |
| **Wi-Fi** | 2.4GHz 802.11 b/g/n |
| **手机** | Android 13+，需 Root 加载 `snd-virmidi` 内核模块 |

> 推荐开发板: ESP32-S3-USB-OTG (N16R8 版本)、ESP32-S3-DevKitC-1 N16R8

## 快速开始

### 方式一：使用预编译固件（推荐）

从 [GitHub Actions](https://github.com/XUE2026/MIDI-Link/actions) 下载最新构建产物:

```
github/actions/{build_id}/产物/
  ├── esp32-midi-gateway.bin          # 主固件 (~800KB)
  ├── bootloader/bootloader.bin       # 启动加载器 (~22KB)
  ├── partition_table/partition-table.bin  # 分区表 (~3KB)
  └── ota_data_initial.bin            # OTA 初始化数据 (~8KB)
```

使用 `esptool.py` 烧录:

```bash
# 擦除全片
esptool.py --chip esp32s3 --port /dev/ttyACM0 erase_flash

# 烧录固件
esptool.py --chip esp32s3 --port /dev/ttyACM0 \
  --baud 921600 write_flash \
  0x0 bootloader/bootloader.bin \
  0x8000 partition_table/partition-table.bin \
  0x10000 esp32-midi-gateway.bin \
  0x310000 ota_data_initial.bin
```

### 方式二：自行编译

```bash
# 克隆仓库
git clone https://github.com/XUE2026/MIDI-Link.git
cd MIDI-Link/esp32-midi-gateway

# 设置 ESP-IDF 环境 (v5.0+)
. /opt/esp/idf/export.sh

# 编译
idf.py set-target esp32s3
idf.py build

# 烧录
idf.py -p /dev/ttyACM0 flash monitor
```

### 手机端配置

```bash
# 在 Termux proot-debian 中
apt install python3 python3-pip alsa-utils
pip3 install python-rtmidi

# 加载虚拟 MIDI 内核模块 (需要 Root)
modprobe snd-virmidi

# 运行接收脚本
python3 phone-receiver/midi_receiver.py
```

> 接收脚本默认监听 UDP 端口 `9000`，自动查找 `Midi Through` 或 `Virtual Raw MIDI` 端口注入。

## 配置

### Web 管理后台
1. 确保电脑/手机已连接至 ESP32-S3 的 Wi-Fi (SSID: `MIDI-LINK-XXXX`)
2. 浏览器访问 `http://192.168.3.1:8088`
3. 默认无密码（生产环境请启用鉴权）

### Wi-Fi 模式切换
```bash
# 通过 SSH 连接 (默认密码: XUE2026)
ssh root@192.168.3.1 -p 22

# 切换为 Station 模式
network_manager_set_mode station

# 切换为 SoftAP 模式
network_manager_set_mode softap
```

## 项目结构

```
MIDI-Link/
├── esp32-midi-gateway/          # ESP32-S3 固件 (ESP-IDF v5.0)
│   ├── main/
│   │   ├── main.c               # 入口/任务初始化
│   │   ├── midi_engine.c/h      # MIDI 引擎 (USB Host + BLE)
│   │   ├── network_manager.c/h  # Wi-Fi 管理 (SoftAP/Station)
│   │   ├── udp_transport.c/h    # UDP/OSC 转发
│   │   ├── web_server.c/h       # Web 管理后台 (8088)
│   │   ├── tcp_control.c/h      # TCP 控制通道 (32)
│   │   ├── ota_manager.c/h      # OTA 升级与回滚
│   │   ├── reset_manager.c/h    # 三重确认重置
│   │   ├── auth_manager.c/h     # 鉴权模块
│   │   └── config_manager.c/h   # NVS 配置存储
│   ├── components/
│   │   └── cjson/               # cJSON 本地组件
│   ├── web_assets/              # Web 前端
│   │   ├── index.html
│   │   ├── style.css
│   │   └── script.js
│   ├── CMakeLists.txt
│   └── sdkconfig.defaults
├── phone-receiver/              # 手机端接收脚本
│   └── midi_receiver.py         # UDP → ALSA MIDI 注入
├── .github/workflows/
│   └── build.yml                # CI 自动构建
└── docs/plans/                  # 设计文档
```

## CI/CD

由 GitHub Actions 自动构建，每次推送到 `development` 分支触发:

- **目标**: `esp32s3` (ESP-IDF v5.0)
- **产物**: 完整 build 目录(含 `.bin`、`.elf`、`.map`)上传为 Artifact
- **质量门**: `phone-receiver/` Python 脚本语法检查
- **保留**: 30 天

## 安全说明

> ⚠️ **本项目为原型/教学用途，生产部署前请注意以下限制:**

| 问题 | 说明 |
|------|------|
| **SSH 未实现** | `ssh_service.c` 仅为占位文件，生产环境缺少 SSH 功能 |
| **弱鉴权** | `auth_manager_check_ip()` 仅做简单 IP 子串匹配(`"192.168.3."`)，极易绕过，且无传输加密 |
| **凭证硬编码** | 默认 SSID 密码 `midi1234`、SSH 密码 `XUE2026` 直接暴露在代码中 |
| **错误处理** | `udp_transport_send()` 发送失败仅记录日志；`config_manager` 保存无重试机制 |
| **代码健壮性** | 多处 `strcpy`、`strstr` 存在缓冲区溢出和逻辑漏洞风险 |

### 生产加固建议
1. 启用 HTTPS (ESP-HTTPD + mbedTLS)
2. 实现完整 SSH 服务或移除依赖
3. 使用加密认证而非 IP 子串匹配
4. 凭证移至 NVS 或外部配置
5. 所有数据通道启用加密

## 开发指南

### 依赖
- ESP-IDF v5.0 或更高版本
- Python 3.8+
- CMake 3.16+
- Ninja 构建系统

### 本地调试

```bash
# 监控串口日志
idf.py monitor

# 清理重建
idf.py fullclean && idf.py build

# 运行指定测试
idf.py build && idf.py -p /dev/ttyACM0 flash monitor
```

### 添加新功能
1. 在 `main/` 下创建 `.c` / `.h` 文件
2. 更新 `main/CMakeLists.txt` 的 `SRCS` 列表
3. 在 `main.c` 的 `app_main()` 中初始化任务

## 许可

本项目基于 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。

**第三方组件:**
- [cJSON](https://github.com/DaveGamble/cJSON) - MIT 许可证
- [ESP-IDF](https://github.com/espressif/esp-idf) - Apache 2.0 许可证