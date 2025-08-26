#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// ============================================================================
// LOGGING SYSTEM
// ============================================================================

// Log levels (higher number = more verbose)
enum class LogLevel {
    ERROR = 0,
    WARN  = 1,
    INFO  = 2,
    DEBUG = 3
};

// Global log level (can be changed at runtime)
extern LogLevel currentLogLevel;

// Initialize logging system
void initLogger(LogLevel level = LogLevel::INFO);

// Set log level at runtime
void setLogLevel(LogLevel level);

// Get current log level
LogLevel getLogLevel();

// Logging macros with automatic level checking
#define LOG_ERROR(fmt, ...)   do { if (currentLogLevel >= LogLevel::ERROR) { Serial.printf("[ERROR] " fmt "\n", ##__VA_ARGS__); } } while(0)
#define LOG_WARN(fmt, ...)    do { if (currentLogLevel >= LogLevel::WARN)  { Serial.printf("[WARN]  " fmt "\n", ##__VA_ARGS__); } } while(0)
#define LOG_INFO(fmt, ...)    do { if (currentLogLevel >= LogLevel::INFO)  { Serial.printf("[INFO]  " fmt "\n", ##__VA_ARGS__); } } while(0)
#define LOG_DEBUG(fmt, ...)   do { if (currentLogLevel >= LogLevel::DEBUG) { Serial.printf("[DEBUG] " fmt "\n", ##__VA_ARGS__); } } while(0)

// Simple logging macros without formatting
#define LOG_ERROR_MSG(msg)    do { if (currentLogLevel >= LogLevel::ERROR) { Serial.println("[ERROR] " msg); } } while(0)
#define LOG_WARN_MSG(msg)     do { if (currentLogLevel >= LogLevel::WARN)  { Serial.println("[WARN]  " msg); } } while(0)
#define LOG_INFO_MSG(msg)     do { if (currentLogLevel >= LogLevel::INFO)  { Serial.println("[INFO]  " msg); } } while(0)
#define LOG_DEBUG_MSG(msg)    do { if (currentLogLevel >= LogLevel::DEBUG) { Serial.println("[DEBUG] " msg); } } while(0)

// Specialized logging macros for different components
#define LOG_AUDIO_ERROR(fmt, ...)   LOG_ERROR("[AUDIO] " fmt, ##__VA_ARGS__)
#define LOG_AUDIO_WARN(fmt, ...)    LOG_WARN("[AUDIO] " fmt, ##__VA_ARGS__)
#define LOG_AUDIO_INFO(fmt, ...)    LOG_INFO("[AUDIO] " fmt, ##__VA_ARGS__)
#define LOG_AUDIO_DEBUG(fmt, ...)   LOG_DEBUG("[AUDIO] " fmt, ##__VA_ARGS__)

#define LOG_RFID_ERROR(fmt, ...)    LOG_ERROR("[RFID] " fmt, ##__VA_ARGS__)
#define LOG_RFID_WARN(fmt, ...)     LOG_WARN("[RFID] " fmt, ##__VA_ARGS__)
#define LOG_RFID_INFO(fmt, ...)     LOG_INFO("[RFID] " fmt, ##__VA_ARGS__)
#define LOG_RFID_DEBUG(fmt, ...)    LOG_DEBUG("[RFID] " fmt, ##__VA_ARGS__)

#define LOG_SETUP_ERROR(fmt, ...)   LOG_ERROR("[SETUP] " fmt, ##__VA_ARGS__)
#define LOG_SETUP_WARN(fmt, ...)    LOG_WARN("[SETUP] " fmt, ##__VA_ARGS__)
#define LOG_SETUP_INFO(fmt, ...)    LOG_INFO("[SETUP] " fmt, ##__VA_ARGS__)
#define LOG_SETUP_DEBUG(fmt, ...)   LOG_DEBUG("[SETUP] " fmt, ##__VA_ARGS__)

#define LOG_DAC_ERROR(fmt, ...)     LOG_ERROR("[DAC] " fmt, ##__VA_ARGS__)
#define LOG_DAC_WARN(fmt, ...)      LOG_WARN("[DAC] " fmt, ##__VA_ARGS__)
#define LOG_DAC_INFO(fmt, ...)      LOG_INFO("[DAC] " fmt, ##__VA_ARGS__)
#define LOG_DAC_DEBUG(fmt, ...)     LOG_DEBUG("[DAC] " fmt, ##__VA_ARGS__)

#define LOG_SD_ERROR(fmt, ...)      LOG_ERROR("[SD] " fmt, ##__VA_ARGS__)
#define LOG_SD_WARN(fmt, ...)       LOG_WARN("[SD] " fmt, ##__VA_ARGS__)
#define LOG_SD_INFO(fmt, ...)       LOG_INFO("[SD] " fmt, ##__VA_ARGS__)
#define LOG_SD_DEBUG(fmt, ...)      LOG_DEBUG("[SD] " fmt, ##__VA_ARGS__)

#define LOG_BATTERY_ERROR(fmt, ...) LOG_ERROR("[BATTERY] " fmt, ##__VA_ARGS__)
#define LOG_BATTERY_WARN(fmt, ...)  LOG_WARN("[BATTERY] " fmt, ##__VA_ARGS__)
#define LOG_BATTERY_INFO(fmt, ...)  LOG_INFO("[BATTERY] " fmt, ##__VA_ARGS__)
#define LOG_BATTERY_DEBUG(fmt, ...) LOG_DEBUG("[BATTERY] " fmt, ##__VA_ARGS__)

#define LOG_BUTTON_ERROR(fmt, ...)  LOG_ERROR("[BUTTON] " fmt, ##__VA_ARGS__)
#define LOG_BUTTON_WARN(fmt, ...)   LOG_WARN("[BUTTON] " fmt, ##__VA_ARGS__)
#define LOG_BUTTON_INFO(fmt, ...)   LOG_INFO("[BUTTON] " fmt, ##__VA_ARGS__)
#define LOG_BUTTON_DEBUG(fmt, ...)  LOG_DEBUG("[BUTTON] " fmt, ##__VA_ARGS__)

#define LOG_ROTARY_ERROR(fmt, ...)  LOG_ERROR("[ROTARY] " fmt, ##__VA_ARGS__)
#define LOG_ROTARY_WARN(fmt, ...)   LOG_WARN("[ROTARY] " fmt, ##__VA_ARGS__)
#define LOG_ROTARY_INFO(fmt, ...)   LOG_INFO("[ROTARY] " fmt, ##__VA_ARGS__)
#define LOG_ROTARY_DEBUG(fmt, ...)  LOG_DEBUG("[ROTARY] " fmt, ##__VA_ARGS__)

#define LOG_SETTINGS_ERROR(fmt, ...) LOG_ERROR("[SETTINGS] " fmt, ##__VA_ARGS__)
#define LOG_SETTINGS_WARN(fmt, ...)  LOG_WARN("[SETTINGS] " fmt, ##__VA_ARGS__)
#define LOG_SETTINGS_INFO(fmt, ...)  LOG_INFO("[SETTINGS] " fmt, ##__VA_ARGS__)
#define LOG_SETTINGS_DEBUG(fmt, ...) LOG_DEBUG("[SETTINGS] " fmt, ##__VA_ARGS__)

#define LOG_MAPPING_ERROR(fmt, ...) LOG_ERROR("[MAPPING] " fmt, ##__VA_ARGS__)
#define LOG_MAPPING_WARN(fmt, ...)  LOG_WARN("[MAPPING] " fmt, ##__VA_ARGS__)
#define LOG_MAPPING_INFO(fmt, ...)  LOG_INFO("[MAPPING] " fmt, ##__VA_ARGS__)
#define LOG_MAPPING_DEBUG(fmt, ...) LOG_DEBUG("[MAPPING] " fmt, ##__VA_ARGS__)

#define LOG_SCANNER_ERROR(fmt, ...) LOG_ERROR("[SCANNER] " fmt, ##__VA_ARGS__)
#define LOG_SCANNER_WARN(fmt, ...)  LOG_WARN("[SCANNER] " fmt, ##__VA_ARGS__)
#define LOG_SCANNER_INFO(fmt, ...)  LOG_INFO("[SCANNER] " fmt, ##__VA_ARGS__)
#define LOG_SCANNER_DEBUG(fmt, ...) LOG_DEBUG("[SCANNER] " fmt, ##__VA_ARGS__)

#endif // LOGGER_H
