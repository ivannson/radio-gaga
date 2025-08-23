#include "Settings_Manager.h"
#include <SD_MMC.h>

// Define static constexpr members
constexpr float Settings_Manager::DEFAULT_VOLUME;
constexpr int Settings_Manager::DEFAULT_SLEEP_TIMEOUT;
constexpr int Settings_Manager::DEFAULT_BATTERY_INTERVAL;
constexpr size_t Settings_Manager::MAX_JSON_SIZE;

// Constructor
Settings_Manager::Settings_Manager(const char* file_path) 
    : settingsFilePath(file_path), fileSystem(nullptr), 
      settingsLoaded(false), fileExists(false) {
    
    // Initialize error buffer
    strcpy(lastError, "No error");
    
    // Initialize with default values
    resetToDefaults();
}

// Initialize settings manager
bool Settings_Manager::begin(void* fs) {
    Serial.println("Initializing Settings Manager...");
    Serial.printf("Settings file: %s\n", settingsFilePath);
    
    // Check if settings file exists
    fileExists = SD_MMC.exists(settingsFilePath);
    
    if (fileExists) {
        Serial.println("Settings file found, attempting to load...");
        if (loadSettings()) {
            Serial.println("Settings loaded successfully!");
            return true;
        } else {
            Serial.printf("Failed to load settings: %s\n", getLastError());
            Serial.println("Creating new settings file with defaults...");
        }
    } else {
        Serial.println("No settings file found, creating defaults...");
    }
    
    // Create default settings file
    if (createDefaultSettings()) {
        Serial.println("Default settings file created successfully!");
        return true;
    } else {
        Serial.printf("Failed to create default settings: %s\n", getLastError());
        return false;
    }
}

// Load settings from file
bool Settings_Manager::loadSettings() {
    if (!fileExists) {
        setLastError("Settings file does not exist");
        return false;
    }
    
    File file = SD_MMC.open(settingsFilePath, "r");
    if (!file) {
        setLastError("Failed to open settings file for reading");
        return false;
    }
    
    // Read file content
    size_t fileSize = file.size();
    if (fileSize > MAX_JSON_SIZE) {
        setLastError("Settings file too large");
        file.close();
        return false;
    }
    
    uint8_t* jsonBuffer = new uint8_t[fileSize + 1];
    if (!jsonBuffer) {
        setLastError("Failed to allocate memory for JSON");
        file.close();
        return false;
    }
    
    size_t bytesRead = file.read(jsonBuffer, fileSize);
    file.close();
    jsonBuffer[bytesRead] = '\0';
    
    // Parse JSON - convert uint8_t* to const char*
    bool success = parseJsonDocument((const char*)jsonBuffer);
    
    // Clean up
    delete[] jsonBuffer;
    
    if (success) {
        settingsLoaded = true;
        Serial.println("Settings parsed successfully");
        printSettings();
    }
    
    return success;
}

// Save settings to file
bool Settings_Manager::saveSettings() {
    Serial.println("Saving settings to file...");
    
    // Create backup first
    if (fileExists) {
        backupSettings();
    }
    
    // Serialize settings to JSON
    char jsonBuffer[MAX_JSON_SIZE];
    if (!serializeToJson(jsonBuffer, MAX_JSON_SIZE)) {
        setLastError("Failed to serialize settings to JSON");
        return false;
    }
    
    // Write to file
    File file = SD_MMC.open(settingsFilePath, "w");
    if (!file) {
        setLastError("Failed to open settings file for writing");
        return false;
    }
    
    size_t bytesWritten = file.print(jsonBuffer);
    file.close();
    
    if (bytesWritten == 0) {
        setLastError("Failed to write settings to file");
        return false;
    }
    
    fileExists = true;
    Serial.println("Settings saved successfully!");
    return true;
}

// Create default settings file
bool Settings_Manager::createDefaultSettings() {
    Serial.println("Creating default settings...");
    
    // Reset to defaults
    resetToDefaults();
    
    // Save to file
    if (saveSettings()) {
        settingsLoaded = true;
        return true;
    }
    
    return false;
}

// Set default volume
void Settings_Manager::setDefaultVolume(float volume) {
    currentSettings.defaultVolume = constrain(volume, 0.0f, 1.0f);
    Serial.printf("Default volume set to: %.2f\n", currentSettings.defaultVolume);
}

// Set WiFi SSID
void Settings_Manager::setWifiSSID(const char* ssid) {
    if (ssid && strlen(ssid) < sizeof(currentSettings.wifiSSID)) {
        strcpy(currentSettings.wifiSSID, ssid);
        Serial.printf("WiFi SSID set to: %s\n", currentSettings.wifiSSID);
    } else {
        Serial.println("Invalid WiFi SSID (too long or null)");
    }
}

// Set WiFi password
void Settings_Manager::setWifiPassword(const char* password) {
    if (password && strlen(password) < sizeof(currentSettings.wifiPassword)) {
        strcpy(currentSettings.wifiPassword, password);
        Serial.printf("WiFi password set to: %s\n", currentSettings.wifiPassword);
    } else {
        Serial.println("Invalid WiFi password (too long or null)");
    }
}

// Set sleep timeout
void Settings_Manager::setSleepTimeout(int minutes) {
    currentSettings.sleepTimeout = constrain(minutes, 1, 1440); // 1 min to 24 hours
    Serial.printf("Sleep timeout set to: %d minutes\n", currentSettings.sleepTimeout);
}

// Set battery check interval
void Settings_Manager::setBatteryCheckInterval(int minutes) {
    currentSettings.batteryCheckInterval = constrain(minutes, 1, 60); // 1 min to 1 hour
    Serial.printf("Battery check interval set to: %d minutes\n", currentSettings.batteryCheckInterval);
}

// Update all settings at once
void Settings_Manager::updateSettings(const Settings& newSettings) {
    currentSettings = newSettings;
    Serial.println("All settings updated");
    printSettings();
}

// Validate settings
bool Settings_Manager::validateSettings() const {
    // Check volume range
    if (currentSettings.defaultVolume < 0.0f || currentSettings.defaultVolume > 1.0f) {
        return false;
    }
    
    // Check timeout values
    if (currentSettings.sleepTimeout < 1 || currentSettings.batteryCheckInterval < 1) {
        return false;
    }
    
    // Check string lengths
    if (strlen(currentSettings.wifiSSID) >= sizeof(currentSettings.wifiSSID)) {
        return false;
    }
    
    if (strlen(currentSettings.wifiPassword) >= sizeof(currentSettings.wifiPassword)) {
        return false;
    }
    
    return true;
}

// Print current settings
void Settings_Manager::printSettings() const {
    Serial.println("\n=== Current Settings ===");
    Serial.printf("Default Volume: %.2f\n", currentSettings.defaultVolume);
    Serial.printf("WiFi SSID: %s\n", currentSettings.wifiSSID[0] ? currentSettings.wifiSSID : "<not set>");
    Serial.printf("WiFi Password: %s\n", currentSettings.wifiPassword[0] ? "***" : "<not set>");
    Serial.printf("Sleep Timeout: %d minutes\n", currentSettings.sleepTimeout);
    Serial.printf("Battery Check Interval: %d minutes\n", currentSettings.batteryCheckInterval);
    Serial.println("========================\n");
}

// Reset to defaults
void Settings_Manager::resetToDefaults() {
    currentSettings = Settings(); // Use default constructor
    Serial.println("Settings reset to defaults");
}

// Backup settings
bool Settings_Manager::backupSettings(const char* backupPath) {
    if (!fileExists) {
        Serial.println("No settings file to backup");
        return true;
    }
    
    File sourceFile = SD_MMC.open(settingsFilePath, "r");
    if (!sourceFile) {
        setLastError("Failed to open source file for backup");
        return false;
    }
    
    File backupFile = SD_MMC.open(backupPath, "w");
    if (!backupFile) {
        setLastError("Failed to create backup file");
        sourceFile.close();
        return false;
    }
    
    // Copy file content
    size_t bytesCopied = 0;
    while (sourceFile.available()) {
        int byte = sourceFile.read();
        if (byte != -1) {
            backupFile.write(byte);
            bytesCopied++;
        }
    }
    
    sourceFile.close();
    backupFile.close();
    
    Serial.printf("Settings backed up to: %s (%d bytes)\n", backupPath, bytesCopied);
    return bytesCopied > 0;
}

// Restore from backup
bool Settings_Manager::restoreFromBackup(const char* backupPath) {
    if (!SD_MMC.exists(backupPath)) {
        setLastError("Backup file does not exist");
        return false;
    }
    
    // Load from backup
    const char* originalPath = settingsFilePath;
    settingsFilePath = backupPath;
    
    bool success = loadSettings();
    
    // Restore original path
    settingsFilePath = originalPath;
    
    if (success) {
        Serial.println("Settings restored from backup successfully!");
        // Save to main settings file
        return saveSettings();
    }
    
    return false;
}

// Get last error
const char* Settings_Manager::getLastError() const {
    return lastError;
}

// Parse JSON document
bool Settings_Manager::parseJsonDocument(const char* jsonString) {
    StaticJsonDocument<MAX_JSON_SIZE> doc;
    
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        setLastError(error.c_str());
        return false;
    }
    
    // Validate JSON structure
    if (!validateJsonStructure(doc)) {
        return false;
    }
    
    // Extract values
    if (doc.containsKey("defaultVolume")) {
        currentSettings.defaultVolume = doc["defaultVolume"] | DEFAULT_VOLUME;
    }
    
    if (doc.containsKey("wifiSSID")) {
        strcpy(currentSettings.wifiSSID, doc["wifiSSID"] | "");
    }
    
    if (doc.containsKey("wifiPassword")) {
        strcpy(currentSettings.wifiPassword, doc["wifiPassword"] | "");
    }
    
    if (doc.containsKey("sleepTimeout")) {
        currentSettings.sleepTimeout = doc["sleepTimeout"] | DEFAULT_SLEEP_TIMEOUT;
    }
    
    if (doc.containsKey("batteryCheckInterval")) {
        currentSettings.batteryCheckInterval = doc["batteryCheckInterval"] | DEFAULT_BATTERY_INTERVAL;
    }
    
    return true;
}

// Serialize to JSON
bool Settings_Manager::serializeToJson(char* buffer, size_t bufferSize) const {
    StaticJsonDocument<MAX_JSON_SIZE> doc;
    
    doc["defaultVolume"] = currentSettings.defaultVolume;
    doc["wifiSSID"] = currentSettings.wifiSSID;
    doc["wifiPassword"] = currentSettings.wifiPassword;
    doc["sleepTimeout"] = currentSettings.sleepTimeout;
    doc["batteryCheckInterval"] = currentSettings.batteryCheckInterval;
    
    size_t bytesWritten = serializeJsonPretty(doc, buffer, bufferSize);
    return bytesWritten > 0;
}

// Validate JSON structure
bool Settings_Manager::validateJsonStructure(JsonDocument& doc) const {
    // Check if it's an object
    if (!doc.is<JsonObject>()) {
        setLastError("Settings file must contain a JSON object");
        return false;
    }
    
    // All fields are optional, so no validation needed
    return true;
}

// Set last error
void Settings_Manager::setLastError(const char* error) const {
    if (error) {
        strncpy(lastError, error, sizeof(lastError) - 1);
        lastError[sizeof(lastError) - 1] = '\0';
    } else {
        strcpy(lastError, "Unknown error");
    }
}
