#include "Logger.h"

// ============================================================================
// LOGGING SYSTEM IMPLEMENTATION
// ============================================================================

// Global log level (default to INFO for production)
LogLevel currentLogLevel = LogLevel::INFO;

// Initialize logging system
void initLogger(LogLevel level) {
    currentLogLevel = level;
    LOG_INFO("Logging system initialized with level: %d", (int)level);
}

// Set log level at runtime
void setLogLevel(LogLevel level) {
    currentLogLevel = level;
    LOG_INFO("Log level changed to: %d", (int)level);
}

// Get current log level
LogLevel getLogLevel() {
    return currentLogLevel;
}
