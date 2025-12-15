#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Settings structure to hold all configuration values
struct Settings {
    // Audio settings
    float defaultVolume;
    float maxVolume;
    
    // WiFi settings
    char wifiSSID[32];
    char wifiPassword[64];
    
    // Power management
    int sleepTimeout;           // minutes
    int batteryCheckInterval;   // minutes
    
    // Constructor with default values
    Settings() : 
        defaultVolume(0.2f),
        maxVolume(1.0f),
        sleepTimeout(15),
        batteryCheckInterval(1) {
        // Initialize string arrays
        strcpy(wifiSSID, "");
        strcpy(wifiPassword, "");
    }
};

class Settings_Manager {
private:
    // Settings file path
    const char* settingsFilePath;
    
    // Current settings in memory
    Settings currentSettings;
    
    // File system reference (will be passed from SD_Manager)
    void* fileSystem;
    
    // Status flags
    bool settingsLoaded;
    bool fileExists;
    
    // Default settings values
    static constexpr float DEFAULT_VOLUME = 0.2f;
    static constexpr float DEFAULT_MAX_VOLUME = 1.0f;
    static constexpr int DEFAULT_SLEEP_TIMEOUT = 15;
    static constexpr int DEFAULT_BATTERY_INTERVAL = 1;
    static constexpr size_t MAX_JSON_SIZE = 1024;

public:
    // Constructor
    Settings_Manager(const char* file_path = "/settings.json");
    
    // Initialize settings manager
    bool begin(void* fs = nullptr);
    
    // Load settings from file
    bool loadSettings();
    
    // Save settings to file
    bool saveSettings();
    
    // Create default settings file
    bool createDefaultSettings();
    
    // Get settings
    const Settings& getSettings() const { return currentSettings; }
    float getDefaultVolume() const { return currentSettings.defaultVolume; }
    float getMaxVolume() const { return currentSettings.maxVolume; }
    const char* getWifiSSID() const { return currentSettings.wifiSSID; }
    const char* getWifiPassword() const { return currentSettings.wifiPassword; }
    int getSleepTimeout() const { return currentSettings.sleepTimeout; }
    int getBatteryCheckInterval() const { return currentSettings.batteryCheckInterval; }
    
    // Set settings
    void setDefaultVolume(float volume);
    void setMaxVolume(float volume);
    void setWifiSSID(const char* ssid);
    void setWifiPassword(const char* password);
    void setSleepTimeout(int minutes);
    void setBatteryCheckInterval(int minutes);
    
    // Update all settings at once
    void updateSettings(const Settings& newSettings);
    
    // Validation
    bool validateSettings() const;
    bool isSettingsLoaded() const { return settingsLoaded; }
    bool doesFileExist() const { return fileExists; }
    
    // Utility functions
    void printSettings() const;
    void resetToDefaults();
    
    // File operations
    bool backupSettings(const char* backupPath = "/settings_backup.json");
    bool restoreFromBackup(const char* backupPath = "/settings_backup.json");
    
    // Error handling
    const char* getLastError() const;
    
private:
    // Internal helper functions
    bool parseJsonDocument(const char* jsonString);
    bool serializeToJson(char* buffer, size_t bufferSize) const;
    bool validateJsonStructure(JsonDocument& doc) const;
    void setLastError(const char* error) const; // Make const-correct
    
    // Error tracking
    mutable char lastError[128]; // Make mutable so it can be modified in const functions
};

#endif // SETTINGS_MANAGER_H
