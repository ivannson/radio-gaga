#ifndef DAC_MANAGER_H
#define DAC_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_TLV320DAC3100.h>

// Headphone routing types removed - now handled in main.cpp
// typedef enum {
//   HP_ROUTE_SPEAKER = 0,
//   HP_ROUTE_HEADPHONES = 1
// } hp_route_t;

class DAC_Manager {
private:
    // DAC object
    Adafruit_TLV320DAC3100 codec;
    
    // Configuration
    uint8_t resetPin;
    uint8_t sdaPin;
    uint8_t sclPin;
    uint8_t i2cAddress;
    bool initialized;
    
    // Headphone detection settings
    // Remove headphone detection - now handled in main.cpp
    // bool enableHeadphoneDetection;
    bool enableSpeakerOutput;
    uint8_t defaultHeadphoneVolume;
    uint8_t defaultSpeakerVolume;
    
    // Default configuration
    static constexpr uint8_t kDefaultHeadphoneVolume = 6;
    static constexpr uint8_t kDefaultSpeakerVolume = 0;
    
    // Remove complex headphone routing state - now handled in main.cpp
    // tlv320_headset_status_t hp_lastRaw = TLV320_HEADSET_NONE;
    // bool hp_filtered = false;
    // uint32_t hp_rawSinceMs = 0;
    // uint32_t hp_lastRouteMs = 0;
    // uint32_t hp_lastRearmMs = 0;

public:
    // Constructor with default pins
    DAC_Manager(uint8_t reset_pin = 4, uint8_t sda_pin = 22, uint8_t scl_pin = 21, uint8_t address = 0x18);
    
    // Initialize the DAC
    bool begin();
    
    // Configuration
    bool configureBasic();
    bool configureFull();
    bool configure(bool enable_headphone_detection, bool enable_speaker_output, 
                  uint8_t headphone_volume, uint8_t speaker_volume);
    
    // Testing and diagnostics
    // Remove testDACRegisters - no longer needed
    // bool testDACRegisters();
    
    // Status and control
    // Remove all headphone-related methods - now handled in main.cpp
    // tlv320_headset_status_t getHeadphoneStatus();
    void enableSpeaker(bool enable);
    void setHeadphoneVolume(uint8_t volume);
    void setSpeakerVolume(uint8_t volume);
    
    // Remove complex headphone routing methods - now handled in main.cpp
    // void updateHeadphoneRouting(bool force = false);
    // tlv320_headset_status_t getHeadphoneStatusRaw();
    // hp_route_t getCurrentRoute() const { return hp_filtered ? HP_ROUTE_HEADPHONES : HP_ROUTE_SPEAKER; }
    
    // Get DAC object for advanced operations
    Adafruit_TLV320DAC3100* getCodec() { return &codec; }
    
    // Check if DAC is initialized
    bool isInitialized() const { return initialized; }
    
    // Check I2C communication
    bool checkI2CCommunication();
    
    // Reset DAC
    void reset();
};

#endif // DAC_MANAGER_H 