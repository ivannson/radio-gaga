#ifndef ROTARY_MANAGER_H
#define ROTARY_MANAGER_H

#include <Arduino.h>
// Forward declaration to avoid enum conflict
class AiEsp32RotaryEncoder;

class Rotary_Manager {
private:
    // Pin definitions
    uint8_t clkPin;
    uint8_t dtPin;
    uint8_t buttonPin;
    uint8_t vccPin;
    
    // Encoder instance
    AiEsp32RotaryEncoder* encoder;
    
    // Volume control
    float currentVolume;
    float minVolume;
    float maxVolume;
    int16_t encoderValue;
    int16_t lastEncoderValue;
    
    // Settings
    bool accelerationEnabled;
    uint16_t accelerationValue;
    bool boundariesSet;
    
    // Callback function for volume changes
    void (*volumeChangeCallback)(float volume);

public:
    // Constructor
    Rotary_Manager(uint8_t clk_pin, uint8_t dt_pin, uint8_t button_pin, uint8_t vcc_pin = -1);
    
    // Destructor
    ~Rotary_Manager();
    
    // Initialization
    bool begin();
    void setup();
    
    // Volume control
    void setVolume(float volume);
    float getVolume() const;
    void setVolumeRange(float min_vol, float max_vol);
    
    // Encoder configuration
    void setAcceleration(uint16_t acceleration);
    void enableAcceleration();
    void disableAcceleration();
    void setConservativeMode(bool enabled); // Disable acceleration for precise control
    uint16_t getAcceleration() const { return accelerationValue; }
    bool isAccelerationEnabled() const { return accelerationEnabled; }
    void setBoundaries(int16_t min_val, int16_t max_val, bool circle = false);
    
    // Update function (call in main loop)
    void update();
    
    // Volume change callback
    void setVolumeChangeCallback(void (*callback)(float volume));
    
    // Utility functions
    int16_t getEncoderValue() const;
    bool isButtonClicked() const;
    void reset();
    
    // ISR function (must be static for interrupt)
    static void IRAM_ATTR readEncoderISR();
    
    // Static instance for ISR access
    static Rotary_Manager* instance;
};

#endif // ROTARY_MANAGER_H
