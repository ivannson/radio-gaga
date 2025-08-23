#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include "SD_MMC.h"

class SD_Manager {
private:
    bool mounted;
    bool initialized;
    
    // SD card configuration
    bool oneBitMode;
    const char* mountPoint;
    
    // Card information
    uint64_t cardSizeMB;
    uint8_t cardType;
    
    // File listing settings
    bool skipSystemDirs;
    int maxFilesToList;

public:
    // Constructor with default settings
    SD_Manager(bool one_bit_mode = true, const char* mount_point = "/sdcard");
    
    // Initialize the SD card
    bool begin();
    
    // Check if SD card is mounted and working
    bool isMounted() const { return mounted; }
    bool isInitialized() const { return initialized; }
    
    // Get card information
    uint64_t getCardSizeMB() const { return cardSizeMB; }
    uint8_t getCardType() const { return cardType; }
    
    // List files in directory
    bool listFiles(const char* path = "/", bool show_system_dirs = false, int max_files = 50);
    
    // Check if file exists
    bool fileExists(const char* path);
    
    // Check if directory exists
    bool directoryExists(const char* path);
    
    // Get file size
    size_t getFileSize(const char* path);
    
    // Open file for reading
    File openFile(const char* path, const char* mode = "r");
    
    // Create directory
    bool createDirectory(const char* path);
    
    // Delete file
    bool deleteFile(const char* path);
    
    // Get free space
    uint64_t getFreeSpace();
    
    // Get used space
    uint64_t getUsedSpace();
    
    // Print card information
    void printCardInfo();
    
    // Set file listing options
    void setSkipSystemDirs(bool skip) { skipSystemDirs = skip; }
    void setMaxFilesToList(int max) { maxFilesToList = max; }
    
    // Get SD_MMC object for advanced operations
    // Note: SD_MMC is a global object, not a class, so we can't return a pointer to it
    // Use SD_MMC directly for advanced operations
};

#endif // SD_MANAGER_H 