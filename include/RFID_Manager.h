#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

// RFID MFRC522 Reset Pin
#define MFRC522_RST_PIN  16  // RST

// Forward declaration
class Audio_Manager;

// Audio control callback function type
typedef void (*AudioControlCallback)(const char* uid, bool tagPresent, bool isNewTag, bool isSameTag);

class RFID_Manager {
private:
    MFRC522 mfrc522;
    bool initialized;
    
    // SPI pin configuration
    uint8_t sclk_pin;
    uint8_t miso_pin;
    uint8_t mosi_pin;
    uint8_t ss_pin;
    
    // Tag presence tracking variables
    bool tagPresent;
    byte lastDetectedUID[10]; // Store the UID of the currently present tag
    byte lastDetectedUIDSize;
    String lastDetectedUIDString; // String representation for easy comparison
    
    // Debounce variables (based on MicroPython code)
    const int DEBOUNCE_THRESHOLD = 5; // Number of iterations to wait (from MicroPython)
    int debounceCounter;
    unsigned long lastTagCheck;
    const int TAG_CHECK_INTERVAL_MS = 100; // Check every 100ms (from MicroPython)
    
    // Audio control variables
    AudioControlCallback audioCallback;
    bool audioControlEnabled;
    
    // Helper functions
    bool compareUid(const byte* uid, const byte* candidate, byte len);
    String uidToString(const byte* uid, byte len);
    
public:
    RFID_Manager(uint8_t sclk, uint8_t miso, uint8_t mosi, uint8_t ss);
    
    // Initialization
    bool begin();
    bool isInitialized() const { return initialized; }
    
    // Main update function - call this in main loop
    void update();
    
    // Status queries
    bool isTagPresent() const { return tagPresent; }
    byte* getLastDetectedUID() { return lastDetectedUID; }
    byte getLastDetectedUIDSize() const { return lastDetectedUIDSize; }
    String getLastDetectedUIDString() const { return lastDetectedUIDString; }
    
    // Audio control
    void setAudioControlCallback(AudioControlCallback callback);
    void enableAudioControl(bool enable) { audioControlEnabled = enable; }
    bool isAudioControlEnabled() const { return audioControlEnabled; }
    
    // Debug and status
    void printStatus();
    void printTagUID();
};

#endif // RFID_MANAGER_H
