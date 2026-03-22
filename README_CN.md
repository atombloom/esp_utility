# ESP32 Utility

基于 ESP-IDF 的通用工具组件，为 ESP32 项目提供统一的时间处理、CPU 使用率统计、Base64 编解码、UUID 生成及 SNTP 时间同步等常用功能，封装在一个 `Utility` 静态类中，方便在各个业务模块中直接调用。

## 特性概览

- **时间工具**
  - 获取系统运行时间（毫秒级）
  - 获取 / 设置 Unix 时间戳（秒 / 毫秒）
  - ISO8601 时间字符串（包含微秒与时区信息）
  - 本地时间字符串与时间戳相互转换（默认 `CST-8`，可自定义时区）
  - 人类可读的中文时间描述（例如：`早上8点10分`）

- **CPU 使用率统计**
  - 某个 FreeRTOS 任务的 CPU 使用率
  - 指定 CPU 核（0/1）整体使用率

- **数据编码与格式化**
  - Base64 编码 / 解码（基于 `mbedtls`）
  - 可靠的 `int64_t` / `uint64_t` 整数转十进制字符串

- **其他工具**
  - 随机 UUID v4 生成（基于 `esp_fill_random`）
  - SNTP 自动时间同步（支持多个国内外 NTP 服务器）

## 环境与依赖

- **ESP-IDF 版本**: `>= 5.3`
- **组件依赖**:
  - `esp_timer`
  - `mbedtls`
- **许可证**: MIT  
- **仓库地址**: `https://github.com/atombloom/esp_utility`

如需使用任务 / CPU 使用率统计，请在 `menuconfig` 中启用：

- `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS`

## 集成方式

1. 将本组件放入工程的 `components/esp_utility` 目录（或通过 git submodule 引入）。
2. 在主工程中正常使用 CMake 构建，`idf.py build` 会自动识别该组件。

组件自身的 `CMakeLists.txt` 片段如下（仅供参考）：

```cmake
idf_component_register(
    SRCS
        "utility.cc"
    INCLUDE_DIRS
        "include"
    EMBED_TXTFILES
    REQUIRES
        "esp_timer"
        "mbedtls"
)
```

`idf_component.yml` 中的元数据示例：

```yaml
dependencies:
  idf: '>=5.3'
description: ESP32 Utility
license: MIT
repository: https://github.com/atombloom/esp_utility
url: https://github.com/atombloom/esp_utility
version: 1.0.0
```

## 基本使用示例

```cpp
#include "utility.h"

void ExampleUsage()
{
    // 获取从系统启动到当前的毫秒数
    uint32_t system_uptime_ms = Utility::GetSystemTimeMs();

    // 获取当前 Unix 时间戳（秒 / 毫秒）
    int64_t timestamp_sec = Utility::GetTimestamp();
    int64_t timestamp_ms  = Utility::GetTimestampMs();

    // 将本地时间字符串转换为时间戳（默认 CST-8）
    int64_t parsed_ts = Utility::LocalTimeStrToTimestamp("2024-10-22T10:40:29");

    // 将时间戳转换为本地时间字符串
    std::string local_time_str = Utility::LocalTimestampToTimeStr(parsed_ts);

    // 获取 ISO8601 格式时间字符串（含微秒与 +08:00 时区）
    std::string iso_time_str = Utility::GetTimestampISO8061();

    // 获取当前核 0 的 CPU 使用率（单位：百分比）
    float core0_usage = Utility::GetCoreUsage(0);

    // 生成 UUID v4 字符串
    std::string uuid_str = Utility::GenerateUuid();

    // Base64 编码 / 解码
    const uint8_t raw_data[] = {1, 2, 3, 4};
    std::string base64_encoded = Utility::Base64Encode(raw_data, sizeof(raw_data));
    std::vector<uint8_t> base64_decoded =
        Utility::Base64Decode(base64_encoded.c_str(), base64_encoded.size());

    // 启动 SNTP 时间同步（每 1 小时同步一次）
    Utility::SntpSyncLaunch(3600);
}
```

## 注意事项

- 如使用 CPU 使用率统计相关接口，请确保在 `menuconfig` 中启用了CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
- 与时间相关的功能依赖 SNTP 或其他方式确保系统时间已经同步，否则返回的时间戳或本地时间字符串可能不准确。
- Base64 解码接口在输入非法字符串或长度为 0 时会返回空的 `std::vector<uint8_t>`，同时输出错误日志，可根据返回值进行错误处理。

## 更新

### 1.0.0
* 版本提交

### 1.0.1
* 更新api


