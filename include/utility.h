#ifndef _UTILITY_H
#define _UTILITY_H_

#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <string>
#include <vector>

class Utility {
public:
    Utility();
    ~Utility();
    static uint32_t GetSystemTimeMs();
    static int64_t GetTimestamp();
    static void SetTimestamp(int64_t timestamp);
    static int64_t GetTimestampMs();
    static void SetTimestampMs(int64_t timestamp_ms);
    static float GetTaskCpuUsage(const char* task_name);
    static float GetCoreUsage(int core_id);
    static std::string Base64Encode(const uint8_t* data, size_t size);
    static std::vector<uint8_t> Base64Decode(const char* base64_str, size_t base64_len);
    static std::string GetTimestampISO8061();
    static int64_t LocalTimeStrToTimestamp(const char* time_str, const char* timezone = "CST-8");
    static std::string LocalTimestampToTimeStr(int64_t timestamp);
    static std::string FormatInt64(int64_t value);
    static std::string FormatUint64(uint64_t value);
    static std::string GenerateUuid();
    static void SntpSyncLaunch(const int sync_interval_s = 3600);
    static std::string LocalTimeStrHM();
    static std::string LocalTimeStrHMS();
    static std::string LocalTimeHumanReadableString();
    static std::string GetMacNumberString();
    static std::string GetSnByMac();

private:
};

#endif // _UTILITY_H_