#ifndef MAPPING_STORE_H
#define MAPPING_STORE_H

#include <Arduino.h>
#include <SD_MMC.h>
#include <unordered_map>
#include <vector>

// Custom hash function for Arduino String class
namespace std {
    template<>
    struct hash<String> {
        size_t operator()(const String& s) const {
            // Simple but effective hash function for Arduino String
            size_t hash = 5381;
            const char* str = s.c_str();
            int c;
            while ((c = *str++)) {
                hash = ((hash << 5) + hash) + c; // hash * 33 + c
            }
            return hash;
        }
    };
}

// Mapping structure for UID to path relationships
struct Mapping {
    String uid;   // uppercase hex
    String path;  // normalized absolute path
    
    Mapping() : uid(""), path("") {}
    Mapping(const String& u, const String& p) : uid(u), path(p) {}
    
    bool isValid() const {
        return uid.length() > 0 && path.length() > 0;
    }
};

class MappingStore {
private:
    fs::FS* sd;
    const char* filePath;
    bool initialized;
    
    // In-memory indexes
    std::unordered_map<String, String> uid_to_path;  // UID -> PATH
    std::unordered_map<String, String> path_to_uid;  // PATH -> UID (most recent binding)
    
    // Helper functions
    bool parseLine(const String& line, Mapping& out) const;
    bool writeCanonical();
    bool flushAndRename(const String& tempPath, const String& finalPath);
    String normalizePath(const String& p) const;
    String normalizeUid(const String& uid) const;
    String extract(const String& line, const String& start, const String& end) const;
    bool createIfMissing();
    
public:
    MappingStore();
    
    // Initialization
    bool begin(fs::FS& sd, const char* path = "/lookup.ndjson");
    bool isInitialized() const { return initialized; }
    
    // Core operations
    bool loadAll();  // builds maps
    bool append(const Mapping& m);         // atomic append
    bool rebind(const String& uid, const String& newPath); // overwrite: rewrite canonical file
    bool unassign(const String& uid);      // remove mapping: rewrite canonical file
    
    // Bijection enforcement
    bool enforceBijection(const String& uid, const String& path);
    bool removePathMapping(const String& path);
    
    // Queries
    bool getPathFor(const String& uid, String& out) const;
    bool getUidFor(const String& path, String& out) const;
    const std::unordered_map<String, String>& uidMap() const { return uid_to_path; }
    const std::unordered_map<String, String>& pathMap() const { return path_to_uid; }
    
    // Utility
    bool hasUid(const String& uid) const;
    bool hasPath(const String& path) const;
    void clear();
    size_t size() const { return uid_to_path.size(); }
    
    // Debug
    void printMappings() const;
    void printStats() const;
};

#endif // MAPPING_STORE_H
