/*
 * Settings_Manager Usage Examples
 * 
 * This file demonstrates how to use the Settings_Manager class
 * for managing JSON configuration files on the SD card.
 */

#include "Settings_Manager.h"

// Create settings manager instance
Settings_Manager settingsManager("/settings.json");

void setup() {
    Serial.begin(115200);
    Serial.println("Settings Manager Test");
    
    // Initialize settings manager
    if (!settingsManager.begin()) {
        Serial.println("Failed to initialize settings manager!");
        while(1) delay(1000);
    }
    
    // Demonstrate getting current settings
    Serial.println("\n=== Current Settings ===");
    float volume = settingsManager.getDefaultVolume();
    const char* ssid = settingsManager.getWifiSSID();
    int sleepTimeout = settingsManager.getSleepTimeout();
    
    Serial.printf("Default Volume: %.2f\n", volume);
    Serial.printf("WiFi SSID: %s\n", ssid[0] ? ssid : "<not set>");
    Serial.printf("Sleep Timeout: %d minutes\n", sleepTimeout);
    
    // Demonstrate updating settings
    Serial.println("\n=== Updating Settings ===");
    settingsManager.setDefaultVolume(0.3f);
    settingsManager.setWifiSSID("MyHomeNetwork");
    settingsManager.setWifiPassword("SecurePassword123");
    settingsManager.setSleepTimeout(30);
    settingsManager.setBatteryCheckInterval(5);
    
    // Save updated settings
    if (settingsManager.saveSettings()) {
        Serial.println("Settings updated and saved successfully!");
    } else {
        Serial.printf("Failed to save settings: %s\n", settingsManager.getLastError());
    }
    
    // Demonstrate backup and restore
    Serial.println("\n=== Backup and Restore Test ===");
    if (settingsManager.backupSettings("/settings_backup.json")) {
        Serial.println("Backup created successfully!");
        
        // Modify a setting
        settingsManager.setDefaultVolume(0.8f);
        settingsManager.saveSettings();
        Serial.println("Modified volume to 0.8");
        
        // Restore from backup
        if (settingsManager.restoreFromBackup("/settings_backup.json")) {
            Serial.println("Settings restored from backup!");
            Serial.printf("Volume restored to: %.2f\n", settingsManager.getDefaultVolume());
        } else {
            Serial.println("Failed to restore from backup!");
        }
    }
    
    // Demonstrate validation
    Serial.println("\n=== Settings Validation ===");
    if (settingsManager.validateSettings()) {
        Serial.println("All settings are valid!");
    } else {
        Serial.println("Some settings are invalid!");
    }
    
    // Demonstrate reset to defaults
    Serial.println("\n=== Reset to Defaults ===");
    settingsManager.resetToDefaults();
    settingsManager.saveSettings();
    Serial.println("Settings reset to defaults and saved!");
    
    Serial.println("\n=== Settings Manager Test Complete ===");
}

void loop() {
    // Settings manager doesn't need continuous updates
    delay(1000);
}

/*
 * Advanced Usage Examples:
 * 
 * // Create settings with custom file path
 * Settings_Manager customSettings("/config/my_settings.json");
 * 
 * // Update multiple settings at once
 * Settings newSettings;
 * newSettings.defaultVolume = 0.5f;
 * newSettings.wifiSSID = "NewNetwork";
 * newSettings.sleepTimeout = 60;
 * settingsManager.updateSettings(newSettings);
 * 
 * // Check if settings are loaded
 * if (settingsManager.isSettingsLoaded()) {
 *     Serial.println("Settings are ready to use");
 * }
 * 
 * // Get all settings as a structure
 * const Settings& currentSettings = settingsManager.getSettings();
 * Serial.printf("Volume: %.2f\n", currentSettings.defaultVolume);
 * 
 * // Error handling
 * if (!settingsManager.loadSettings()) {
 *     Serial.printf("Error: %s\n", settingsManager.getLastError());
 * }
 */
