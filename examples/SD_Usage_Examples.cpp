/*
 * SD_Manager Usage Examples
 * 
 * This file shows different ways to configure and use the SD_Manager class
 */

#include "SD_Manager.h"

// Example 1: Basic setup with default settings
void example1_basicSetup() {
    // Create SD manager with default settings (1-bit mode, /sdcard mount point)
    SD_Manager sd;
    
    // Initialize SD card
    if (sd.begin()) {
        Serial.println("SD card initialized successfully!");
        
        // List files in root directory
        sd.listFiles("/", false, 20);  // path, show_system_dirs, max_files
    }
}

// Example 2: Custom configuration
void example2_customConfig() {
    // Create SD manager with custom settings
    SD_Manager sd(false, "/sdcard");  // 4-bit mode, custom mount point
    
    if (sd.begin()) {
        Serial.println("SD card initialized with custom settings!");
    }
}

// Example 3: File operations
void example3_fileOperations() {
    SD_Manager sd;
    
    if (sd.begin()) {
        // Check if file exists
        if (sd.fileExists("/settings.json")) {
            Serial.println("settings.json exists");
            
            // Get file size
            size_t size = sd.getFileSize("/settings.json");
            Serial.printf("File size: %d bytes\n", size);
        }
        
        // Check if directory exists
        if (sd.directoryExists("/music")) {
            Serial.println("music directory exists");
            
            // List files in music directory
            sd.listFiles("/music", false, 30);
        }
        
        // Open and read a file
        File file = sd.openFile("/settings.json", "r");
        if (file) {
            Serial.println("File opened successfully");
            // Read file content here
            file.close();
        }
    }
}

// Example 4: Directory operations
void example4_directoryOperations() {
    SD_Manager sd;
    
    if (sd.begin()) {
        // Create a new directory
        if (sd.createDirectory("/new_folder")) {
            Serial.println("Directory created successfully");
            
            // List files to see the new directory
            sd.listFiles("/", false, 50);
        }
        
        // Check if directory was created
        if (sd.directoryExists("/new_folder")) {
            Serial.println("Directory exists!");
        }
    }
}

// Example 5: File management
void example5_fileManagement() {
    SD_Manager sd;
    
    if (sd.begin()) {
        // Create a test file (this would need to be implemented)
        // For now, just check if a file exists and delete it
        
        if (sd.fileExists("/test.txt")) {
            Serial.println("test.txt exists, deleting...");
            if (sd.deleteFile("/test.txt")) {
                Serial.println("File deleted successfully");
            }
        }
    }
}

// Example 6: Card information
void example6_cardInfo() {
    SD_Manager sd;
    
    if (sd.begin()) {
        // Get card information
        uint64_t size = sd.getCardSizeMB();
        uint8_t type = sd.getCardType();
        
        Serial.printf("Card size: %llu MB\n", size);
        Serial.printf("Card type: %d\n", type);
        
        // Print detailed card info
        sd.printCardInfo();
    }
}

// Example 7: Advanced file listing
void example7_advancedListing() {
    SD_Manager sd;
    
    if (sd.begin()) {
        // Configure listing options
        sd.setSkipSystemDirs(true);  // Skip directories starting with .
        sd.setMaxFilesToList(100);   // List up to 100 files
        
        // List files with system directories shown
        Serial.println("Listing with system directories:");
        sd.listFiles("/", true, 50);
        
        // List files without system directories
        Serial.println("Listing without system directories:");
        sd.listFiles("/", false, 50);
        
        // List files in a specific directory
        if (sd.directoryExists("/music")) {
            Serial.println("Listing music directory:");
            sd.listFiles("/music", false, 20);
        }
    }
}

// Example 8: Error handling
void example8_errorHandling() {
    SD_Manager sd;
    
    // Check if SD card is mounted
    if (!sd.isMounted()) {
        Serial.println("SD card not mounted!");
        return;
    }
    
    // Check if SD card is initialized
    if (!sd.isInitialized()) {
        Serial.println("SD card not initialized!");
        return;
    }
    
    // Try to access a file that doesn't exist
    if (!sd.fileExists("/nonexistent.txt")) {
        Serial.println("File does not exist (expected)");
    }
    
    // Try to open a file that doesn't exist
    File file = sd.openFile("/nonexistent.txt", "r");
    if (!file) {
        Serial.println("Could not open file (expected)");
    }
    
    Serial.println("Error handling test complete");
}

// Example 9: Space management
void example9_spaceManagement() {
    SD_Manager sd;
    
    if (sd.begin()) {
        // Get free space (approximate)
        uint64_t freeSpace = sd.getFreeSpace();
        Serial.printf("Free space: %llu bytes\n", freeSpace);
        
        // Get used space (approximate)
        uint64_t usedSpace = sd.getUsedSpace();
        Serial.printf("Used space: %llu bytes\n", usedSpace);
        
        // Note: These are approximations as SD_MMC doesn't provide direct space info
    }
}

// Example 10: Integration with other managers
void example10_integration() {
    SD_Manager sd;
    DAC_Manager dac;
    
    // Initialize both managers
    if (sd.begin() && dac.begin()) {
        Serial.println("Both SD and DAC initialized successfully!");
        
        // Check if both are ready
        if (sd.isMounted() && dac.isInitialized()) {
            Serial.println("System ready for audio playback!");
            
            // List audio files
            if (sd.directoryExists("/music")) {
                Serial.println("Audio files found:");
                sd.listFiles("/music", false, 30);
            }
        }
    }
} 