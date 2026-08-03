#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_INFO  3
#define LOG_LEVEL_DEBUG 4

#ifndef CURRENT_LOG_LEVEL
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
#endif

class LoggerStream {
public:
    LoggerStream(const char* prefix, int level)
        : prefix_(prefix), level_(level), enabled_(level_ <= CURRENT_LOG_LEVEL) {
        if (enabled_) {
            Serial.print(prefix_);
            Serial.print(" ");
        }
    }

    template <typename T>
    LoggerStream& operator<<(const T& value) {
        if (enabled_) {
            Serial.print(value);
        }
        return *this;
    }

    ~LoggerStream() {
        if (enabled_) {
            Serial.println();
        }
    }

private:
    const char* prefix_;
    int level_;
    bool enabled_;
};

#define LOG_ERROR(expr) do { LoggerStream _log("[ERROR]", LOG_LEVEL_ERROR); _log << expr; } while (0)
#define LOG_WARN(expr)  do { LoggerStream _log("[WARN]",  LOG_LEVEL_WARN);  _log << expr; } while (0)
#define LOG_INFO(expr)  do { LoggerStream _log("[INFO]",  LOG_LEVEL_INFO);  _log << expr; } while (0)
#define LOG_DEBUG(expr) do { LoggerStream _log("[DEBUG]", LOG_LEVEL_DEBUG); _log << expr; } while (0)

#endif