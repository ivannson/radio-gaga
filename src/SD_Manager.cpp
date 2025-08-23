#include "SD_Manager.h"
#include "driver/gpio.h"

// Constructor
SD_Manager::SD_Manager(bool one_bit_mode, const char* mount_point) {
    oneBitMode = one_bit_mode;
    mountPoint = mount_point;
    mounted = false;
    initialized = false;
    cardSizeMB = 0;
    cardType = CARD_NONE;
    skipSystemDirs = true;
    maxFilesToList = 50;
}

// Initialize the SD card
bool SD_Manager::begin() {
    Serial.println("Initializing SD card...");
    
    // Enable pull-up for the data line we're using (D0)
    if (oneBitMode) {
        gpio_pullup_en(GPIO_NUM_2);   // D0
        Serial.println("Enabled pull-up for D0 (1-bit mode)");
    }
    
    // Mount SD card
    if (!SD_MMC.begin(mountPoint, oneBitMode)) {
        Serial.println("SD card mount failed!");
        return false;
    }
    
    // Check card type
    cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached!");
        return false;
    }
    
    // Get card size
    cardSizeMB = SD_MMC.cardSize() / (1024 * 1024);
    
    // Print card info
    printCardInfo();
    
    mounted = true;
    initialized = true;
    Serial.println("SD card initialized successfully!");
    return true;
}

// Print card information
void SD_Manager::printCardInfo() {
    Serial.printf("SD Card Size: %lluMB\n", cardSizeMB);
    
    Serial.print("Card Type: ");
    switch (cardType) {
        case CARD_MMC:
            Serial.println("MMC");
            break;
        case CARD_SD:
            Serial.println("SDSC");
            break;
        case CARD_SDHC:
            Serial.println("SDHC");
            break;
        default:
            Serial.println("UNKNOWN");
            break;
    }
}

// List files in directory
bool SD_Manager::listFiles(const char* path, bool show_system_dirs, int max_files) {
    if (!mounted) {
        Serial.println("SD card not mounted!");
        return false;
    }
    
    Serial.printf("\n=== Files in %s ===\n", path);
    File root = SD_MMC.open(path);
    if (!root) {
        Serial.println("Failed to open directory!");
        return false;
    }
    
    if (!root.isDirectory()) {
        Serial.println("Path is not a directory!");
        root.close();
        return false;
    }
    
    File file = root.openNextFile();
    int fileCount = 0;
    int dirCount = 0;
    int totalCount = 0;
    
    while (file && totalCount < max_files) {
        if (!file.isDirectory()) {
            fileCount++;
            Serial.printf("%d. %s (%d bytes)\n", fileCount, file.name(), file.size());
        } else {
            // Skip system directories that start with . if configured
            if (!show_system_dirs && file.name()[0] == '.') {
                // Skip this directory
            } else {
                dirCount++;
                Serial.printf("DIR: %s\n", file.name());
            }
        }
        file.close();
        file = root.openNextFile();
        totalCount++;
    }
    
    root.close();
    Serial.printf("Total: %d files, %d dirs\n", fileCount, dirCount);
    Serial.println("==============================\n");
    
    return true;
}

// Check if file exists
bool SD_Manager::fileExists(const char* path) {
    if (!mounted) return false;
    return SD_MMC.exists(path);
}

// Check if directory exists
bool SD_Manager::directoryExists(const char* path) {
    if (!mounted) return false;
    File file = SD_MMC.open(path);
    if (!file) return false;
    bool isDir = file.isDirectory();
    file.close();
    return isDir;
}

// Get file size
size_t SD_Manager::getFileSize(const char* path) {
    if (!mounted) return 0;
    File file = SD_MMC.open(path);
    if (!file) return 0;
    size_t size = file.size();
    file.close();
    return size;
}

// Open file for reading
File SD_Manager::openFile(const char* path, const char* mode) {
    if (!mounted) return File();
    return SD_MMC.open(path, mode);
}

// Create directory
bool SD_Manager::createDirectory(const char* path) {
    if (!mounted) return false;
    return SD_MMC.mkdir(path);
}

// Delete file
bool SD_Manager::deleteFile(const char* path) {
    if (!mounted) return false;
    return SD_MMC.remove(path);
}

// Get free space (approximate)
uint64_t SD_Manager::getFreeSpace() {
    if (!mounted) return 0;
    // Note: SD_MMC doesn't have a direct free space method
    // This is a placeholder - you might need to implement this differently
    return cardSizeMB * 1024 * 1024; // Return total size as approximation
}

// Get used space (approximate)
uint64_t SD_Manager::getUsedSpace() {
    if (!mounted) return 0;
    // Note: SD_MMC doesn't have a direct used space method
    // This is a placeholder - you might need to implement this differently
    return 0; // Would need to calculate by scanning files
} 