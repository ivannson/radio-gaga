#ifndef SD_SCANNER_H
#define SD_SCANNER_H

#include <Arduino.h>
#include <SD_MMC.h>
#include <vector>

class SdScanner {
private:
    fs::FS* sd;
    bool initialized;
    
public:
    SdScanner();
    
    // Initialization
    bool begin(fs::FS& sd);
    bool isInitialized() const { return initialized; }
    
    // Directory scanning
    bool listAudioDirs(fs::FS& sd, const String& root, std::vector<String>& out);
    bool listAudioDirsRecursive(fs::FS& sd, const String& root, std::vector<String>& out, int depth = 1, int maxDepth = 2);
    
    // Utility functions
    static bool isHiddenOrSystem(const String& name);
    static String normalizePath(const String& p);
    
    // Debug
    void printScannedDirs(const std::vector<String>& dirs) const;
};

#endif // SD_SCANNER_H
