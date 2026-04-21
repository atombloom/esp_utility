#include <mbedtls/base64.h>
#include <esp_log.h>
#include <esp_random.h>
#include <esp_sntp.h>
#include <esp_mac.h>

#include <cstring>
#include <ctime>
#include <sys/time.h>

#include "utility.h"

#define TAG "UTILITY"

float Utility::GetTaskCpuUsage(const char* task_name){
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize;
    UBaseType_t x; 
    uint32_t ulTotalRunTime;
    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
    float cpuUse = 0.0F;
    
    // check if the runtime statistics is enabled
    if (ulTotalRunTime == 0) {
        ESP_LOGW(TAG, "GetTaskCpuUsage: Runtime statistics not enabled. Please enable CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS.");
        vPortFree(pxTaskStatusArray);
        return 0.0F;
    }
    
    for (x = 0; x < uxArraySize; x++) {
        if (strcmp(pxTaskStatusArray[x].pcTaskName, task_name) == 0) {
            float tCounter = pxTaskStatusArray[x].ulRunTimeCounter;
            float tCounterAll  = ulTotalRunTime;
            // 避免除以 0 的情况
            if (tCounterAll > 0) {
                cpuUse = tCounter / tCounterAll * 100.0F;
            } else {
                cpuUse = 0.0F;
            }
            break;
        }
    }
    vPortFree(pxTaskStatusArray);
    return cpuUse;
}

float Utility::GetCoreUsage(const int core_id) { // 0 or 1
    float usage = 0.0;
    if (core_id == 0) {
        usage = 100.0 - GetTaskCpuUsage("IDLE0");
    } else if (core_id == 1) {
        usage = 100.0 - GetTaskCpuUsage("IDLE1");
    } else {
        ESP_LOGE(TAG, "Invalid core id: %d", core_id);
    }
    return usage;
}

// the minimum resolution depends on CONFIG_FREERTOS_HZ
// for example, if CONFIG_FREERTOS_HZ == 1000, the minimum resolution is 1ms
uint32_t Utility::GetSystemTimeMs() {
    TickType_t timestamp = xTaskGetTickCount();
    uint32_t milliseconds = timestamp * portTICK_PERIOD_MS;
    return milliseconds;
}

// get the timestamp in seconds (UTC，与时区无关)
int64_t Utility::GetTimestamp() {
    time_t now;
    time(&now);
    return (int64_t)now;
}

// 设置当前时间
// 参数: timestamp 时间戳(秒级)
void Utility::SetTimestamp(int64_t timestamp) {
    struct timeval tv;
    tv.tv_sec = (time_t)(timestamp); 
    tv.tv_usec = (suseconds_t)0;
    settimeofday(&tv, NULL);
}

// get the timestamp in milliseconds (UTC，与时区无关)
int64_t Utility::GetTimestampMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    // 毫秒 = 秒 * 1000 + 微秒 / 1000
    return ((int64_t)tv.tv_sec * 1000) + ((int64_t)tv.tv_usec / 1000);
}

// 设置当前时间
// 参数: timestamp_ms 时间戳(毫秒级)
void Utility::SetTimestampMs(int64_t timestamp_ms) {
    struct timeval tv;
    tv.tv_sec = (time_t)(timestamp_ms / 1000);
    tv.tv_usec = (suseconds_t)((timestamp_ms % 1000) * 1000);
    settimeofday(&tv, NULL);
}

// 不依赖 printf 的 %llu/%lld，在 ESP32 newlib 下可靠打印 64 位整数
std::string Utility::FormatUint64(uint64_t value) {
    if (value == 0) {
        return "0";
    }
    char buf[24];  // 最多 20 位十进制 + '\0'
    char* p = buf + sizeof(buf) - 1;
    *p = '\0';
    while (value > 0) {
        *--p = (char)('0' + (value % 10));
        value /= 10;
    }
    return std::string(p);
}

std::string Utility::FormatInt64(int64_t value) {
    if (value >= 0) {
        return FormatUint64((uint64_t)value);
    }
    return std::string("-") + FormatUint64((uint64_t)(-(value + 1)) + 1);
}

std::string Utility::Base64Encode(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0) {
        return std::string();
    }

    // first call: get the size of the encoded data
    // when dst is nullptr and dlen is 0, the function will return the size of the encoded data
    size_t dlen = 0;
    int ret = mbedtls_base64_encode(nullptr, 0, &dlen, (const unsigned char*)data, size);
    if (ret != 0 && dlen == 0) {
        ESP_LOGE(TAG, "Failed to calculate base64 encode size, ret: %d", ret);
        return std::string();
    }
    
    // second call: encode the data
    std::string result(dlen, 0);
    size_t olen = 0;
    ret = mbedtls_base64_encode((unsigned char*)result.data(), result.size(), &olen, 
                                  (const unsigned char*)data, size);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to encode base64, ret: %d", ret);
        return std::string();
    }
    
    // resize the string to the actual encoded length
    result.resize(olen);
    return result;
}

std::vector<uint8_t> Utility::Base64Decode(const char* base64_str, size_t base64_len) {
    std::vector<uint8_t> result;
    
    if (base64_str == nullptr || base64_len == 0) {
        ESP_LOGE(TAG, "Invalid base64 string or length");
        return result;
    }

    size_t decoded_len = 0;
    int ret = mbedtls_base64_decode(nullptr, 0, &decoded_len, 
                                    (const unsigned char*)base64_str, base64_len);
    if (ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL || decoded_len == 0) {
        ESP_LOGE(TAG, "Failed to calculate base64 decode size, ret: %d, decoded_len: %zu", ret, decoded_len);
        return result;
    }
    
    result.resize(decoded_len);
    size_t output_len = 0;
    ret = mbedtls_base64_decode(result.data(), result.size(), &output_len,
                                (const unsigned char*)base64_str, base64_len);
    if (ret != 0 || output_len == 0) {
        ESP_LOGE(TAG, "Failed to decode base64 data, ret: %d, output_len: %zu", ret, output_len);
        result.clear();
        return result;
    }
    
    result.resize(output_len);
    return result;
}

/**
 * @brief  获取当前时间字符串,时区上海
 * @note
 * @param [in] timestr 传入字符串，注意大小设置为 TOOL_TIMESTR_LENGTH
 * @return  返回时间日期字符串: "2024-10-22T10:40:29.123456+08:00" (ISO 8601 格式)
 */
#define TOOL_TIMESTR_LENGTH 64
std::string Utility::GetTimestampISO8601() {
    char timestr[TOOL_TIMESTR_LENGTH];
    memset(timestr, 0, TOOL_TIMESTR_LENGTH);
    time_t now;
    struct tm timeinfo;
    struct timespec ts;

    // 获取当前时间
    time(&now);
    // 将时区设置为中国标准时间
    setenv("TZ", "CST-8", 1);
    tzset();
    localtime_r(&now, &timeinfo);

    // 获取当前时间的微秒部分
    clock_gettime(CLOCK_REALTIME, &ts);

    // 格式化时间字符串
    strftime(timestr, TOOL_TIMESTR_LENGTH, "%Y-%m-%dT%H:%M:%S", &timeinfo);

    // 添加微秒部分
    snprintf(timestr + strlen(timestr), TOOL_TIMESTR_LENGTH - strlen(timestr), ".%06ld+08:00", ts.tv_nsec / 1000);

    return std::string(timestr);
}

/**
 * @brief UTC秒级时间字符串转成 Unix 时间戳(秒)
 * @param time_str 格式: "2024-10-22T10:40:29" 或 "2024-10-22T10:40"
 *                 (ISO 8601 无时区，默认CST-8时区，北京时间)
 * @return 时间戳(秒)，解析失败或无效时间返回 -1
 */
int64_t Utility::LocalTimeStrToTimestamp(const char* time_str, const char* timezone) {
    if (!time_str || !*time_str) {
        return -1;
    }

    struct tm tm = {};
    int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
    int n = sscanf(time_str, "%d-%d-%dT%d:%d:%d",
                   &year, &month, &day, &hour, &min, &sec);
    // 支持 "YYYY-MM-DDTHH:MM:SS"(6 段) 或 "YYYY-MM-DDTHH:MM"(5 段，秒视为 0)
    if (n == 5) {
        sec = 0;
    } else if (n != 6) {
        ESP_LOGE(TAG, "Parse failed, str=%s", time_str);
        return -1;
    }
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = -1;  // 由系统判断是否夏令时

    // 按北京时间(CST-8)解释，mktime 得到对应 UTC 时间戳
    setenv("TZ", timezone, 1);
    tzset();
    time_t t = mktime(&tm);
    if (t == (time_t)-1) {
        return -1;
    }
    return (int64_t)t;
}

/**
 * @brief Unix 时间戳(秒) 转为时间字符串
 * @param timestamp 秒级 Unix 时间戳
 * @return 格式 "YYYY-MM-DDTHH:MM:SS"，按默认时区 CST-8 的本地时间；失败返回空字符串
 */
std::string Utility::LocalTimestampToTimeStr(int64_t timestamp) {
    time_t t = (time_t)timestamp;
    struct tm timeinfo = {};
    setenv("TZ", "CST-8", 1);
    tzset();
    if (localtime_r(&t, &timeinfo) == nullptr) {
        ESP_LOGE(TAG, "TimestampToUtcTimeStr: localtime_r failed, ts=%lld", (long long)timestamp);
        return std::string();
    }
    char buf[32];
    size_t len = strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    if (len == 0) {
        return std::string();
    }
    return std::string(buf, len);
}

std::string Utility::GenerateUuid() {
    // UUID v4 需要 16 字节的随机数据
    uint8_t uuid[16];
    
    // 使用 ESP32 的硬件随机数生成器
    esp_fill_random(uuid, sizeof(uuid));
    
    // 设置版本 (版本 4) 和变体位
    uuid[6] = (uuid[6] & 0x0F) | 0x40;    // 版本 4
    uuid[8] = (uuid[8] & 0x3F) | 0x80;    // 变体 1
    
    // 将字节转换为标准的 UUID 字符串格式
    char uuid_str[37];
    snprintf(uuid_str, sizeof(uuid_str),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid[0], uuid[1], uuid[2], uuid[3],
        uuid[4], uuid[5], uuid[6], uuid[7],
        uuid[8], uuid[9], uuid[10], uuid[11],
        uuid[12], uuid[13], uuid[14], uuid[15]);
    
    return std::string(uuid_str);
}

/**
 * @brief  开启sntp时间同步
        1.进行ntp时间同步,默认周期为1小时,可能存在同步延迟
 * @note   
 * @param [in] sync_interval_s    时间同步间隔，单位s
 * @return 
 */
void Utility::SntpSyncLaunch(const int sync_interval_s) {
    ESP_LOGI(TAG, "sntp run...");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    //配置多个时间同步服务器
    esp_sntp_setservername(0, "cn.ntp.org.cn");
    esp_sntp_setservername(1, "pool.ntp.org");
    esp_sntp_setservername(2, "ntp.ntsc.ac.cn");
    esp_sntp_setservername(3, "ntp1.aliyun.com");
    esp_sntp_setservername(4, "ntp2.aliyun.com");
    esp_sntp_setservername(5, "ntp3.aliyun.com");
    //sntp初始化
    esp_sntp_init();
    esp_sntp_set_sync_interval(sync_interval_s*1000); //每n ms同步一次
    //注册SNTP同步完成的回调函数
    // esp_sntp_set_time_sync_notification_cb(sntp_sync_notification_cb);
    //等待时间同步完成
    // wait_sntp_sync();
}

// 获取当前时间字符串HH:MM
std::string Utility::LocalTimeStrHM() {
    time_t now;
    time(&now);
    struct tm timeinfo = {};
    localtime_r(&now, &timeinfo);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    return std::string(buf);
}

// 获取当前时间字符串HH:MM:SS
std::string Utility::LocalTimeStrHMS() {
    time_t now;
    time(&now);
    struct tm timeinfo = {};
    localtime_r(&now, &timeinfo);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return std::string(buf);
}

// return local time human readable string
// 例如：早上8点10分，中午12点20分，下午1点30分，晚上9点40分
std::string Utility::LocalTimeHumanReadableString() {
    int64_t timestamp = Utility::GetTimestamp();
    // 1. 检查时间戳是否有效（0通常表示时间未同步）
    if (timestamp == 0) {
        return "";
    }
    time_t t = (time_t)timestamp;
    struct tm timeinfo = {};
    
    // 2. 使用 localtime_r 将时间戳转换为本地时间（线程安全）
    // 注意：ESP32 需要先设置时区，否则默认是 UTC
    localtime_r(&t, &timeinfo);

    const char* prefix = "";
    int hour = timeinfo.tm_hour;

    // 3. 根据你的逻辑判断时间段（基于24小时制）
    if (hour < 5) {
        prefix = "凌晨"; // 建议补充：0-5点通常称为凌晨
    } 
    else if (hour < 9) {
        prefix = "早上"; // 5-9点
    } 
    else if (hour < 12) {
        prefix = "上午"; // 9-12点
    } 
    else if (hour < 13) {
        prefix = "中午"; // 12-13点
    } 
    else if (hour < 19) {
        prefix = "下午"; // 13-19点
    } 
    else {
        prefix = "晚上"; // 19-24点
    }

    // 4. 将24小时制转换为12小时制显示 (例如 13 -> 1)
    int display_hour = hour % 12;
    if (display_hour == 0) {
        display_hour = 12; // 0点或12点显示为12
    }
    // 5. 格式化字符串
    // %02d 表示分钟不足两位时补0 (例如 8点05分)
    char buf[32];
    snprintf(buf, sizeof(buf), "%s%d点%02d分", prefix, display_hour, timeinfo.tm_min);
    return std::string(buf);
}

//MAC to int64_t number string: 1234567890
std::string Utility::GetMacNumberString() {
    uint8_t mac[6];
    esp_err_t ret;
#if CONFIG_IDF_TARGET_ESP32P4
    ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
#else
    ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
#endif
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get MAC address, error: %d", ret);
        return std::string("");
    }
    // 将 6 个字节的 MAC 地址组合成一个 int64_t 数字
    // 使用掩码确保只使用低 48 位，避免符号位被影响，保证结果始终为正数
    int64_t mac_number = (((int64_t)mac[0] << 40) | 
                          ((int64_t)mac[1] << 32) | 
                          ((int64_t)mac[2] << 24) | 
                          ((int64_t)mac[3] << 16) | 
                          ((int64_t)mac[4] << 8) | 
                          (int64_t)mac[5]) & 0x0000FFFFFFFFFFFFLL;
    return std::to_string(mac_number);
}

//MAC to SN string: SN1234567890
std::string Utility::GetSnByMac() {
    std::string mac_sn("SN");
    mac_sn += GetMacNumberString();
    return mac_sn;
}
