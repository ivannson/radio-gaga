# Settings_Manager

A C++ class for managing JSON configuration files on the SD card in the Smart Audio Player project.

## Features

- **JSON Configuration**: Human-readable settings stored in `settings.json`
- **Automatic Creation**: Creates default settings file if none exists
- **Validation**: Ensures all settings are within valid ranges
- **Backup/Restore**: Automatic backup before saving, restore capability
- **Memory Efficient**: Uses ArduinoJson with static memory allocation
- **Error Handling**: Comprehensive error reporting and recovery

## Settings Structure

### Audio Settings
- **`defaultVolume`**: Float (0.0-1.0) - Initial volume on startup

### WiFi Settings
- **`wifiSSID`**: String (max 32 chars) - WiFi network name
- **`wifiPassword`**: String (max 64 chars) - WiFi password

### Power Management
- **`sleepTimeout`**: Integer (1-1440 minutes) - Deep sleep timeout
- **`batteryCheckInterval`**: Integer (1-60 minutes) - Battery check frequency

## File Format

The `settings.json` file is automatically created with this structure:

```json
{
  "defaultVolume": 0.2,
  "wifiSSID": "",
  "wifiPassword": "",
  "sleepTimeout": 15,
  "batteryCheckInterval": 1
}
```

## Usage

### Basic Setup

```cpp
#include "Settings_Manager.h"

// Create instance
Settings_Manager settingsManager("/settings.json");

// Initialize
if (!settingsManager.begin()) {
    Serial.println("Failed to initialize settings!");
    return;
}
```

### Getting Settings

```cpp
// Get individual values
float volume = settingsManager.getDefaultVolume();
const char* ssid = settingsManager.getWifiSSID();
int sleepTimeout = settingsManager.getSleepTimeout();

// Get all settings at once
const Settings& currentSettings = settingsManager.getSettings();
```

### Updating Settings

```cpp
// Update individual settings
settingsManager.setDefaultVolume(0.5f);
settingsManager.setWifiSSID("MyNetwork");
settingsManager.setWifiPassword("MyPassword123");
settingsManager.setSleepTimeout(30);

// Save changes
if (settingsManager.saveSettings()) {
    Serial.println("Settings saved successfully!");
}
```

### Advanced Operations

```cpp
// Update multiple settings at once
Settings newSettings;
newSettings.defaultVolume = 0.3f;
newSettings.wifiSSID = "NewNetwork";
settingsManager.updateSettings(newSettings);

// Backup and restore
settingsManager.backupSettings("/settings_backup.json");
settingsManager.restoreFromBackup("/settings_backup.json");

// Reset to defaults
settingsManager.resetToDefaults();
settingsManager.saveSettings();
```

## Integration with Rotary_Manager

The Settings_Manager integrates with the Rotary_Manager to set the initial volume:

```cpp
// In main setup
if (settingsManager.isSettingsLoaded()) {
    float defaultVolume = settingsManager.getDefaultVolume();
    rotaryManager.setVolume(defaultVolume);
    Serial.printf("Volume set from settings: %.2f\n", defaultVolume);
}
```

## Error Handling

```cpp
if (!settingsManager.loadSettings()) {
    Serial.printf("Settings error: %s\n", settingsManager.getLastError());
    
    // Create default settings
    if (settingsManager.createDefaultSettings()) {
        Serial.println("Default settings created");
    }
}
```

## File Operations

### Automatic Backup
- Creates backup before saving changes
- Backup file: `/settings_backup.json`
- Prevents data loss during power failures

### File Validation
- Checks JSON format validity
- Validates setting value ranges
- Ensures string length limits

### Memory Management
- Uses static JSON document (1KB max)
- Efficient string handling
- Minimal dynamic allocation

## Dependencies

- **ArduinoJson**: JSON parsing and serialization
- **SD_MMC**: SD card file system access
- **Arduino Framework**: Core functionality

## Error Codes

Common error messages:
- `"Settings file does not exist"` - File not found
- `"Failed to open settings file for reading"` - File access error
- `"Settings file too large"` - File exceeds 1KB limit
- `"Failed to parse JSON"` - Invalid JSON format
- `"Failed to serialize settings to JSON"` - Memory allocation error

## Performance

- **Initialization**: ~50ms for file operations
- **Load Settings**: ~10ms for JSON parsing
- **Save Settings**: ~20ms for file writing
- **Memory Usage**: ~2KB total (including JSON buffer)

## Future Enhancements

- **Encryption**: Secure storage of sensitive data
- **Remote Updates**: WiFi-based settings synchronization
- **Version Control**: Settings file versioning
- **Schema Validation**: Strict JSON schema enforcement
- **Auto-save**: Automatic saving on value changes
