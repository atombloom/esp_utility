# ESP32 Utility

A general-purpose utility component based on ESP-IDF. It provides unified time handling, CPU usage statistics, Base64 encode/decode, UUID generation, and SNTP time synchronization for ESP32 projects. All functions are wrapped in a `Utility` static class so that business modules can call them directly.

## Features Overview

- **Time Utilities**
  - Get system uptime in milliseconds
  - Get / set Unix timestamp (seconds / milliseconds)
  - ISO8601 time string (including microseconds and timezone information)
  - Convert between local time string and timestamp (default timezone `CST-8`, customizable)
  - Human-readable Chinese time description (for example: `早上8点10分`)

- **CPU Usage Statistics**
  - CPU usage of a specific FreeRTOS task
  - Overall usage of a specific CPU core (0 / 1)

- **Data Encoding and Formatting**
  - Base64 encode / decode (based on `mbedtls`)
  - Reliable `int64_t` / `uint64_t` to decimal string conversion

- **Other Utilities**
  - Random UUID v4 generation (based on `esp_fill_random`)
  - Automatic SNTP time synchronization (supports multiple domestic and international NTP servers)

## Environment and Dependencies

- **ESP-IDF version**: `>= 5.3`
- **Component dependencies**:
  - `esp_timer`
  - `mbedtls`
- **License**: MIT  
- **Repository**: `https://github.com/atombloom/esp_utility`

If you need task / CPU usage statistics, please enable the following option in `menuconfig`:

- `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS`

## Integration

1. Place this component under your project directory as `components/esp_utility` (or add it via git submodule).
2. Build your main project with CMake as usual, `idf.py build` will automatically detect this component.

The `CMakeLists.txt` of this component looks like the following (for reference only):

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

Example metadata in `idf_component.yml`:

```yaml
dependencies:
  idf: '>=5.3'
description: ESP32 Utility
license: MIT
repository: https://github.com/atombloom/esp_utility
url: https://github.com/atombloom/esp_utility
version: 1.0.0
```

## Basic Usage Example

```cpp
#include "utility.h"

void ExampleUsage()
{
    // Get milliseconds from system startup to now
    uint32_t system_uptime_ms = Utility::GetSystemTimeMs();

    // Get current Unix timestamp (seconds / milliseconds)
    int64_t timestamp_sec = Utility::GetTimestamp();
    int64_t timestamp_ms  = Utility::GetTimestampMs();

    // Convert local time string to timestamp (default CST-8)
    int64_t parsed_ts = Utility::LocalTimeStrToTimestamp("2024-10-22T10:40:29");

    // Convert timestamp to local time string
    std::string local_time_str = Utility::LocalTimestampToTimeStr(parsed_ts);

    // Get ISO8601 formatted time string (with microseconds and +08:00 timezone)
    std::string iso_time_str = Utility::GetTimestampISO8061();

    // Get CPU usage of core 0 (percentage)
    float core0_usage = Utility::GetCoreUsage(0);

    // Generate UUID v4 string
    std::string uuid_str = Utility::GenerateUuid();

    // Base64 encode / decode
    const uint8_t raw_data[] = {1, 2, 3, 4};
    std::string base64_encoded = Utility::Base64Encode(raw_data, sizeof(raw_data));
    std::vector<uint8_t> base64_decoded =
        Utility::Base64Decode(base64_encoded.c_str(), base64_encoded.size());

    // Launch SNTP time synchronization (sync every 1 hour)
    Utility::SntpSyncLaunch(3600);
}
```

## Notes

- If you use CPU usage related APIs, please make sure `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS` is enabled in `menuconfig`.
- Time-related functionalities depend on SNTP or other mechanisms to ensure that system time has been synchronized; otherwise, returned timestamps or local time strings may be inaccurate.
- When Base64 decode is called with invalid strings or zero-length input, it returns an empty `std::vector<uint8_t>` and prints an error log. You can check the return value to handle errors.
