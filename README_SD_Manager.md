# SD_Manager Class Documentation

## Overview

The `SD_Manager` class provides a clean, configurable interface for managing SD card operations using the ESP32's SD_MMC interface. It encapsulates all the complex initialization and file operation logic, making it easy to work with SD cards in your projects.

## Features

- **Configurable SD mode** - Support for both 1-bit and 4-bit modes
- **Flexible mount points** - Custom mount point configuration
- **File operations** - Read, write, delete, and directory operations
- **File listing** - Configurable file listing with system directory filtering
- **Error handling** - Comprehensive error checking and reporting
- **Card information** - Access to card size, type, and status
- **Clean interface** - Simple API that hides the complexity of SD_MMC

## Files

- `include/SD_Manager.h` - Header file with class declaration
- `src/SD_Manager.cpp` - Implementation file
- `examples/SD_Usage_Examples.cpp` - Usage examples
- `README_SD_Manager.md` - This documentation

## Quick Start

### Basic Usage

```cpp
#include "SD_Manager.h"

// Create SD manager with default settings
SD_Manager sd;

void setup() {
    Serial.begin(115200);
    
    // Initialize SD card
    if (sd.begin()) {
        Serial.println("SD card ready!");
        
        // List files in root directory
        sd.listFiles("/", false, 20);
    }
}
```

### Custom Configuration

```cpp
// Create SD manager with custom settings
SD_Manager sd(false, "/sdcard");  // 4-bit mode, custom mount point

if (sd.begin()) {
    Serial.println("SD card initialized with custom settings!");
}
```

## API Reference

### Constructor

```cpp
SD_Manager(bool one_bit_mode = true, const char* mount_point = "/sdcard")
```

**Parameters:**
- `one_bit_mode` - Use 1-bit mode if true, 4-bit mode if false (default: true)
- `mount_point` - Mount point for the SD card (default: "/sdcard")

### Initialization

```cpp
bool begin()
```

Initializes the SD card, including mounting and basic communication test.

**Returns:** `true` if successful, `false` if failed

### Status Methods

#### Check Status
```cpp
bool isMounted() const
bool isInitialized() const
```

Returns `true` if the SD card is properly mounted and initialized.

#### Card Information
```cpp
uint64_t getCardSizeMB() const
uint8_t getCardType() const
void printCardInfo()
```

Get card size in MB, card type, and print detailed card information.

### File Operations

#### File Existence and Size
```cpp
bool fileExists(const char* path)
bool directoryExists(const char* path)
size_t getFileSize(const char* path)
```

Check if files/directories exist and get file sizes.

#### File Access
```cpp
File openFile(const char* path, const char* mode = "r")
```

Open a file for reading or writing. Returns a File object.

#### File Management
```cpp
bool createDirectory(const char* path)
bool deleteFile(const char* path)
```

Create directories and delete files.

### File Listing

#### Basic Listing
```cpp
bool listFiles(const char* path = "/", bool show_system_dirs = false, int max_files = 50)
```

List files in a directory with configurable options.

**Parameters:**
- `path` - Directory path to list (default: "/")
- `show_system_dirs` - Include system directories starting with "." (default: false)
- `max_files` - Maximum number of files to list (default: 50)

#### Listing Configuration
```cpp
void setSkipSystemDirs(bool skip)
void setMaxFilesToList(int max)
```

Configure file listing behavior.

### Space Management

```cpp
uint64_t getFreeSpace()
uint64_t getUsedSpace()
```

Get approximate free and used space (note: these are approximations as SD_MMC doesn't provide direct space info).

### Advanced Access

For advanced operations, you can use the global `SD_MMC` object directly:

```cpp
// Example: Direct SD_MMC access
if (SD_MMC.exists("/some_file.txt")) {
    // Advanced operations
}
```

## Pin Mapping

### 1-Bit Mode (Default)
- **CMD**: GPIO 15
- **CLK**: GPIO 14
- **D0**: GPIO 2 (data line)
- **Pull-up**: Automatically enabled on D0

### 4-Bit Mode
- **CMD**: GPIO 15
- **CLK**: GPIO 14
- **D0**: GPIO 2
- **D1**: GPIO 4
- **D2**: GPIO 12
- **D3**: GPIO 13

### Important Notes
- **GPIO 2** is used for SD card data line
- **Pull-up resistors** are automatically enabled for 1-bit mode
- **4-bit mode** requires additional data lines but provides higher performance

## Integration with Main Project

The SD_Manager integrates seamlessly with the main audio player project:

```cpp
// In main.cpp
#include "SD_Manager.h"

// Create SD manager instance
SD_Manager sdManager(true, "/sdcard");  // one_bit_mode, mount_point

void setup() {
    // ... other initialization code ...
    
    // Initialize SD card
    if (!sdManager.begin()) {
        // Handle initialization error
        return;
    }
    
    // List files
    sdManager.listFiles("/", false, 50);
    
    // ... continue with rest of setup ...
}
```

## Error Handling

The SD_Manager includes comprehensive error handling:

1. **Mount Errors** - Checks if SD card can be mounted
2. **Card Type Errors** - Verifies card is present and recognized
3. **File Operation Errors** - Validates file/directory operations
4. **Status Validation** - Ensures SD card is ready before operations

## Common Use Cases

### Audio File Management
```cpp
SD_Manager sd;

if (sd.begin()) {
    // Check for music directory
    if (sd.directoryExists("/music")) {
        // List audio files
        sd.listFiles("/music", false, 30);
        
        // Check for specific audio file
        if (sd.fileExists("/music/song.mp3")) {
            size_t size = sd.getFileSize("/music/song.mp3");
            Serial.printf("Audio file size: %d bytes\n", size);
        }
    }
}
```

### Settings File Management
```cpp
SD_Manager sd;

if (sd.begin()) {
    // Check for settings file
    if (sd.fileExists("/settings.json")) {
        File file = sd.openFile("/settings.json", "r");
        if (file) {
            // Read settings
            // ... read file content ...
            file.close();
        }
    } else {
        // Create default settings
        File file = sd.openFile("/settings.json", "w");
        if (file) {
            // Write default settings
            // ... write file content ...
            file.close();
        }
    }
}
```

### Directory Structure Management
```cpp
SD_Manager sd;

if (sd.begin()) {
    // Create directory structure
    sd.createDirectory("/music");
    sd.createDirectory("/music/playlists");
    sd.createDirectory("/logs");
    
    // List created structure
    sd.listFiles("/", false, 50);
}
```

## Troubleshooting

### Common Issues

1. **"SD card mount failed"**
   - Check SD card is properly inserted
   - Verify card is formatted (FAT32 recommended)
   - Check wiring connections
   - Try different SD card

2. **"No SD card attached"**
   - Check physical connection
   - Verify card is powered
   - Check for bent pins

3. **"Failed to open directory"**
   - Check if path exists
   - Verify permissions
   - Ensure SD card is mounted

4. **File listing issues**
   - Check max_files parameter
   - Verify show_system_dirs setting
   - Ensure directory path is correct

### Debug Output

The SD_Manager provides detailed debug output during initialization and operations. Monitor the serial output to identify where issues occur.

## Performance Considerations

- **1-bit mode** is sufficient for most audio applications
- **4-bit mode** provides higher performance but requires more pins
- **File listing** can be slow with many files - use max_files parameter
- **System directories** are skipped by default for better performance

## Examples

See `examples/SD_Usage_Examples.cpp` for comprehensive usage examples covering:
- Basic setup
- Custom configuration
- File operations
- Directory operations
- File management
- Card information
- Advanced listing
- Error handling
- Space management
- Integration with other managers 