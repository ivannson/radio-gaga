#include "SdScanner.h"

// Constructor
SdScanner::SdScanner() : sd(nullptr), initialized(false) {
}

// Initialize the SD scanner
bool SdScanner::begin(fs::FS& sd) {
    this->sd = &sd;
    initialized = true;
    Serial.println("SdScanner: Initialized");
    return true;
}

// List audio directories under root (recursive)
bool SdScanner::listAudioDirsRecursive(fs::FS& sd, const String& root, std::vector<String>& out, int depth, int maxDepth) {
    if (depth > maxDepth) return true;
    
    if (!sd.exists(root)) {
        Serial.printf("SdScanner: Directory %s does not exist\n", root.c_str());
        return false;
    }
    
    File rootDir = sd.open(root);
    if (!rootDir || !rootDir.isDirectory()) {
        Serial.printf("SdScanner: Failed to open directory %s\n", root.c_str());
        return false;
    }
    
    File file = rootDir.openNextFile();
    int dirCount = 0;
    
    while (file) {
        if (file.isDirectory()) {
            String dirName = String(file.name());
            
            // Skip hidden and system directories
            if (!isHiddenOrSystem(dirName)) {
                String fullPath = normalizePath(root + "/" + dirName);
                out.push_back(fullPath);
                dirCount++;
                
                // Recursively scan subdirectories
                if (depth < maxDepth) {
                    listAudioDirsRecursive(sd, fullPath, out, depth + 1, maxDepth);
                }
            } else {
                Serial.printf("SdScanner: Skipping hidden/system directory: %s\n", dirName.c_str());
            }
        }
        
        file.close();
        file = rootDir.openNextFile();
    }
    
    rootDir.close();
    
    if (depth == 1) { // Only print summary at top level
        Serial.printf("SdScanner: Found %d valid directories (recursive scan, max depth: %d)\n", out.size(), maxDepth);
    }
    
    return true;
}

// List audio directories under root (non-recursive)
bool SdScanner::listAudioDirs(fs::FS& sd, const String& root, std::vector<String>& out) {
    out.clear();
    return listAudioDirsRecursive(sd, root, out, 1, 1);
}

// Check if directory name should be excluded
bool SdScanner::isHiddenOrSystem(const String& name) {
    // Skip names starting with .
    if (name.startsWith(".")) return true;
    
    // Skip known system directories
    String lowerName = name;
    lowerName.toLowerCase();
    
    if (lowerName == "system volume information") return true;
    if (lowerName == "found.000") return true;
    if (lowerName == "recycler") return true;
    if (lowerName == "trash") return true;
    if (lowerName == "lost+found") return true;
    if (lowerName == "windows") return true;
    if (lowerName == "macos") return true;
    if (lowerName == "android") return true;
    
    return false;
}

// Normalize path
String SdScanner::normalizePath(const String& p) {
    String normalized = p;
    
    // Remove any existing leading slashes first
    while (normalized.startsWith("/")) {
        normalized = normalized.substring(1);
    }
    
    // Add single leading slash
    normalized = "/" + normalized;
    
    // Remove trailing slash (but keep root "/")
    if (normalized.endsWith("/") && normalized.length() > 1) {
        normalized = normalized.substring(0, normalized.length() - 1);
    }
    
    return normalized;
}

// Debug: print scanned directories
void SdScanner::printScannedDirs(const std::vector<String>& dirs) const {
    Serial.println("=== Scanned Directories ===");
    for (size_t i = 0; i < dirs.size(); i++) {
        Serial.printf("  %d: %s\n", i, dirs[i].c_str());
    }
    Serial.printf("Total: %d directories\n", dirs.size());
    Serial.println("===========================");
}
