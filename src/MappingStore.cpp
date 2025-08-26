#include "MappingStore.h"
#include "Logger.h"

// Constructor
MappingStore::MappingStore() 
    : sd(nullptr), filePath("/lookup.ndjson"), initialized(false) {
}

// Initialize the mapping store
bool MappingStore::begin(fs::FS& sd, const char* path) {
    this->sd = &sd;
    if (path) this->filePath = path;
    
    LOG_MAPPING_INFO("Initializing with file: %s", this->filePath);
    
    // Try to load existing mappings
    if (!loadAll()) {
        LOG_MAPPING_WARN("Failed to load mappings, creating new file");
        if (!createIfMissing()) {
            LOG_MAPPING_ERROR("Failed to create mapping file");
            return false;
        }
    }
    
    initialized = true;
    LOG_MAPPING_INFO("Initialized with %d mappings", size());
    return true;
}

// Load all mappings from file
bool MappingStore::loadAll() {
    uid_to_path.clear();
    path_to_uid.clear();
    
    if (!sd->exists(filePath)) {
        Serial.println("MappingStore: Mapping file does not exist");
        return false;
    }
    
    File f = sd->open(filePath, FILE_READ);
    if (!f) {
        Serial.println("MappingStore: Failed to open mapping file for reading");
        return false;
    }
    
    int lineCount = 0;
    int validCount = 0;
    
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        lineCount++;
        
        if (line.length() == 0) continue;
        
        Mapping mapping;
        if (parseLine(line, mapping)) {
            uid_to_path[mapping.uid] = mapping.path;
            path_to_uid[mapping.path] = mapping.uid; // last write wins
            validCount++;
        } else {
            Serial.printf("MappingStore: Failed to parse line %d: %s\n", lineCount, line.c_str());
        }
    }
    
    f.close();
    Serial.printf("MappingStore: Loaded %d valid mappings from %d lines\n", validCount, lineCount);
    return true;
}

// Parse a single NDJSON line
bool MappingStore::parseLine(const String& line, Mapping& out) const {
    // Minimal JSON parse: expect {"uid":"..","path":".."}
    String uid = extract(line, "\"uid\":\"", "\"");
    String path = extract(line, "\"path\":\"", "\"");
    
    if (uid.length() == 0 || path.length() == 0) {
        return false;
    }
    
    // Normalize the data
    out.uid = normalizeUid(uid);
    out.path = normalizePath(path);
    
    return out.isValid();
}

// Extract substring between start and end markers
String MappingStore::extract(const String& line, const String& start, const String& end) const {
    int startPos = line.indexOf(start);
    if (startPos == -1) return "";
    
    startPos += start.length();
    int endPos = line.indexOf(end, startPos);
    if (endPos == -1) return "";
    
    return line.substring(startPos, endPos);
}

// Normalize UID to uppercase hex
String MappingStore::normalizeUid(const String& uid) const {
    String normalized = uid;
    normalized.toUpperCase();
    
    // Remove spaces and colons
    normalized.replace(" ", "");
    normalized.replace(":", "");
    
    return normalized;
}

// Normalize path
String MappingStore::normalizePath(const String& p) const {
    String normalized = p;
    
    // Ensure leading slash
    if (!normalized.startsWith("/")) {
        normalized = "/" + normalized;
    }
    
    // Remove trailing slash
    if (normalized.endsWith("/") && normalized.length() > 1) {
        normalized = normalized.substring(0, normalized.length() - 1);
    }
    
    return normalized;
}

// Create empty mapping file if missing
bool MappingStore::createIfMissing() {
    if (sd->exists(filePath)) return true;
    
    File f = sd->open(filePath, FILE_WRITE);
    if (!f) {
        Serial.println("MappingStore: Failed to create mapping file");
        return false;
    }
    
    f.close();
    Serial.println("MappingStore: Created empty mapping file");
    return true;
}

// Append a new mapping (atomic write)
bool MappingStore::append(const Mapping& m) {
    if (!m.isValid()) {
        Serial.println("MappingStore: Invalid mapping for append");
        return false;
    }
    
    // Enforce uniqueness: check if UID or path already exists
    String existingPath;
    if (getPathFor(m.uid, existingPath)) {
        Serial.printf("MappingStore: UID %s already mapped to %s\n", m.uid.c_str(), existingPath.c_str());
        return false; // UID already exists
    }
    
    String existingUid;
    if (getUidFor(m.path, existingUid)) {
        Serial.printf("MappingStore: Path %s already mapped to UID %s\n", m.path.c_str(), existingUid.c_str());
        return false; // Path already exists
    }
    
    // Add to memory maps
    uid_to_path[m.uid] = m.path;
    path_to_uid[m.path] = m.uid;
    
    // Write to file atomically: append new line to existing file
    String tempPath = String(filePath) + ".tmp";
    File tempFile = sd->open(tempPath, FILE_WRITE);
    if (!tempFile) {
        Serial.println("MappingStore: Failed to open temp file for append");
        return false;
    }
    
    // Copy existing content first
    if (sd->exists(filePath)) {
        File existingFile = sd->open(filePath, FILE_READ);
        if (existingFile) {
            while (existingFile.available()) {
                tempFile.write(existingFile.read());
            }
            existingFile.close();
        }
    }
    
    // Append new line
    String line = "{\"uid\":\"" + m.uid + "\",\"path\":\"" + m.path + "\"}\n";
    tempFile.print(line);
    
    // Critical: flush and close
    tempFile.flush();
    tempFile.close();
    
    // Atomic rename
    if (!flushAndRename(tempPath, filePath)) {
        Serial.println("MappingStore: Failed to rename temp file after append");
        return false;
    }
    
    Serial.printf("MappingStore: Appended mapping %s -> %s\n", m.uid.c_str(), m.path.c_str());
    return true;
}

// Rebind UID to new path (overwrite)
bool MappingStore::rebind(const String& uid, const String& newPath) {
    String normalizedUid = normalizeUid(uid);
    String normalizedPath = normalizePath(newPath);
    
    if (normalizedUid.length() == 0 || normalizedPath.length() == 0) {
        Serial.println("MappingStore: Invalid UID or path for rebind");
        return false;
    }
    
    // Check if new path is already mapped to another UID
    String existingUid;
    if (getUidFor(normalizedPath, existingUid)) {
        if (existingUid != normalizedUid) {
            Serial.printf("MappingStore: Path %s already mapped to UID %s\n", 
                        normalizedPath.c_str(), existingUid.c_str());
            return false; // Path conflict - need to handle in calling code
        }
    }
    
    // IMPORTANT: Read and store the old path BEFORE updating the maps
    String oldPath = "";
    auto oldPathIt = uid_to_path.find(normalizedUid);
    if (oldPathIt != uid_to_path.end()) {
        oldPath = oldPathIt->second;
    }
    
    // Update memory maps
    uid_to_path[normalizedUid] = normalizedPath;
    path_to_uid[normalizedPath] = normalizedUid;
    
    // Remove old path mapping if UID was mapped to different path
    if (oldPath.length() > 0 && oldPath != normalizedPath) {
        path_to_uid.erase(oldPath);
        Serial.printf("MappingStore: Removed old path mapping %s\n", oldPath.c_str());
    }
    
    // Rewrite canonical file
    if (!writeCanonical()) {
        Serial.println("MappingStore: Failed to write canonical file after rebind");
        return false;
    }
    
    Serial.printf("MappingStore: Rebound UID %s from %s to %s\n", 
                  normalizedUid.c_str(), oldPath.c_str(), normalizedPath.c_str());
    return true;
}

// Remove mapping for UID
bool MappingStore::unassign(const String& uid) {
    String normalizedUid = normalizeUid(uid);
    
    auto it = uid_to_path.find(normalizedUid);
    if (it == uid_to_path.end()) {
        Serial.printf("MappingStore: UID %s not found for unassign\n", normalizedUid.c_str());
        return false;
    }
    
    String path = it->second;
    
    // Remove from memory maps
    uid_to_path.erase(it);
    path_to_uid.erase(path);
    
    // Rewrite canonical file
    if (!writeCanonical()) {
        Serial.println("MappingStore: Failed to write canonical file after unassign");
        return false;
    }
    
    Serial.printf("MappingStore: Unassigned UID %s from path %s\n", normalizedUid.c_str(), path.c_str());
    return true;
}

// Write canonical file (removes duplicates)
bool MappingStore::writeCanonical() {
    String tempPath = String(filePath) + ".tmp";
    File f = sd->open(tempPath, FILE_WRITE);
    if (!f) {
        Serial.println("MappingStore: Failed to open temp file for canonical write");
        return false;
    }
    
    // Write all mappings
    for (const auto& pair : uid_to_path) {
        String line = "{\"uid\":\"" + pair.first + "\",\"path\":\"" + pair.second + "\"}\n";
        f.print(line);
    }
    
    // Critical: flush and sync to ensure data is written to SD
    f.flush();
    f.close();
    
    // Atomic rename
    if (!flushAndRename(tempPath, filePath)) {
        Serial.println("MappingStore: Failed to rename temp file");
        return false;
    }
    
    return true;
}

// Atomic write helper with proper fsync
bool MappingStore::flushAndRename(const String& tempPath, const String& finalPath) {
    // Remove old file if it exists
    if (sd->exists(finalPath)) {
        if (!sd->remove(finalPath)) {
            Serial.println("MappingStore: Failed to remove old file");
            return false;
        }
    }
    
    // Rename temp to final
    if (!sd->rename(tempPath, finalPath)) {
        Serial.println("MappingStore: Failed to rename temp file");
        return false;
    }
    
    return true;
}

// Query methods
bool MappingStore::getPathFor(const String& uid, String& out) const {
    String normalizedUid = normalizeUid(uid);
    auto it = uid_to_path.find(normalizedUid);
    if (it != uid_to_path.end()) {
        out = it->second;
        return true;
    }
    return false;
}

bool MappingStore::getUidFor(const String& path, String& out) const {
    String normalizedPath = normalizePath(path);
    auto it = path_to_uid.find(normalizedPath);
    if (it != path_to_uid.end()) {
        out = it->second;
        return true;
    }
    return false;
}

bool MappingStore::hasUid(const String& uid) const {
    String normalizedUid = normalizeUid(uid);
    return uid_to_path.find(normalizedUid) != uid_to_path.end();
}

bool MappingStore::hasPath(const String& path) const {
    String normalizedPath = normalizePath(path);
    return path_to_uid.find(normalizedPath) != path_to_uid.end();
}

void MappingStore::clear() {
    uid_to_path.clear();
    path_to_uid.clear();
}

// Debug methods
void MappingStore::printMappings() const {
    Serial.println("=== MappingStore Contents ===");
    for (const auto& pair : uid_to_path) {
        Serial.printf("  %s -> %s\n", pair.first.c_str(), pair.second.c_str());
    }
    Serial.println("=============================");
}

void MappingStore::printStats() const {
    Serial.printf("MappingStore Stats: %d mappings, initialized: %s\n", 
                  size(), initialized ? "yes" : "no");
}

// Enforce bijection: ensure 1:1 UID-to-path relationship
bool MappingStore::enforceBijection(const String& uid, const String& path) {
    String normalizedUid = normalizeUid(uid);
    String normalizedPath = normalizePath(path);
    
    if (normalizedUid.length() == 0 || normalizedPath.length() == 0) {
        Serial.println("MappingStore: Invalid UID or path for bijection enforcement");
        return false;
    }
    
    // Check if path is already mapped to another UID
    String existingUid;
    if (getUidFor(normalizedPath, existingUid)) {
        if (existingUid != normalizedUid) {
            Serial.printf("MappingStore: Path %s already mapped to UID %s\n", 
                        normalizedPath.c_str(), existingUid.c_str());
            return false; // Need to handle this case in calling code
        }
    }
    
    // Check if UID is already mapped to another path
    String existingPath;
    if (getPathFor(normalizedUid, existingPath)) {
        if (existingPath != normalizedPath) {
            Serial.printf("MappingStore: UID %s already mapped to path %s\n", 
                        normalizedUid.c_str(), existingPath.c_str());
            return false; // Need to handle this case in calling code
        }
    }
    
    return true;
}

// Remove path mapping (helper for bijection enforcement)
bool MappingStore::removePathMapping(const String& path) {
    String normalizedPath = normalizePath(path);
    
    auto it = path_to_uid.find(normalizedPath);
    if (it != path_to_uid.end()) {
        String uid = it->second;
        uid_to_path.erase(uid);
        path_to_uid.erase(it);
        Serial.printf("MappingStore: Removed path mapping %s -> %s\n", normalizedPath.c_str(), uid.c_str());
        return true;
    }
    
    return false;
}
