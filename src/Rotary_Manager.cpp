#include "Rotary_Manager.h"
#include "AiEsp32RotaryEncoder.h"

// Static instance for ISR access
Rotary_Manager* Rotary_Manager::instance = nullptr;

// ISR function
void IRAM_ATTR Rotary_Manager::readEncoderISR() {
    if (instance && instance->encoder) {
        instance->encoder->readEncoder_ISR();
    }
}

// Constructor
Rotary_Manager::Rotary_Manager(uint8_t clk_pin, uint8_t dt_pin, uint8_t button_pin, uint8_t vcc_pin)
    : clkPin(clk_pin), dtPin(dt_pin), buttonPin(button_pin), vccPin(vcc_pin),
      encoder(nullptr), currentVolume(0.5f), minVolume(0.0f), maxVolume(1.0f),
      encoderValue(50), lastEncoderValue(50), accelerationEnabled(true),
      accelerationValue(250), boundariesSet(false), volumeChangeCallback(nullptr) {
    
    // Set static instance for ISR access
    instance = this;
}

// Destructor
Rotary_Manager::~Rotary_Manager() {
    if (encoder) {
        delete encoder;
        encoder = nullptr;
    }
    instance = nullptr;
}

// Initialization
bool Rotary_Manager::begin() {
    // Create encoder instance
    encoder = new AiEsp32RotaryEncoder(clkPin, dtPin, buttonPin, vccPin, 4); // 4 steps per detent
    
    if (!encoder) {
        Serial.println("Failed to create rotary encoder instance!");
        return false;
    }
    
    // Initialize encoder
    encoder->begin();
    
    // Setup with ISR
    setup();
    
    // Set default boundaries (0-100)
    setBoundaries(0, 100, false);
    
    // Set acceleration - use a more conservative value to prevent skipping
    if (accelerationEnabled) {
        encoder->setAcceleration(50); // Reduced from 250 to 50 for smoother control
    } else {
        encoder->disableAcceleration();
    }
    
    Serial.println("Rotary encoder: Conservative acceleration enabled");
    Serial.println("Rotary encoder: Boundaries set to 0-100 (no wrap-around)");
    
    // Set initial encoder value based on current volume
    encoderValue = (int16_t)(currentVolume * 100.0f);
    encoder->setEncoderValue(encoderValue);
    lastEncoderValue = encoderValue;
    
    Serial.printf("Rotary encoder initialized on pins CLK:%d, DT:%d, BTN:%d\n", clkPin, dtPin, buttonPin);
    Serial.printf("Initial volume: %.2f (encoder value: %d)\n", currentVolume, encoderValue);
    
    return true;
}

// Setup with ISR
void Rotary_Manager::setup() {
    if (encoder) {
        encoder->setup(readEncoderISR);
    }
}

// Set volume (0.0 to 1.0)
void Rotary_Manager::setVolume(float volume) {
    // Clamp volume to valid range
    volume = constrain(volume, minVolume, maxVolume);
    
    if (volume != currentVolume) {
        currentVolume = volume;
        encoderValue = (int16_t)(volume * 100.0f);
        
        if (encoder) {
            encoder->setEncoderValue(encoderValue);
        }
        
        Serial.printf("Volume set to: %.2f (encoder: %d)\n", currentVolume, encoderValue);
        
        // Call callback if set
        if (volumeChangeCallback) {
            volumeChangeCallback(currentVolume);
        }
    }
}

// Get current volume
float Rotary_Manager::getVolume() const {
    return currentVolume;
}

// Set volume range
void Rotary_Manager::setVolumeRange(float min_vol, float max_vol) {
    minVolume = min_vol;
    maxVolume = max_vol;
    
    // Ensure current volume is within new range
    if (currentVolume < minVolume) {
        setVolume(minVolume);
    } else if (currentVolume > maxVolume) {
        setVolume(maxVolume);
    }
}

// Set acceleration
void Rotary_Manager::setAcceleration(uint16_t acceleration) {
    accelerationValue = acceleration;
    if (encoder) {
        encoder->setAcceleration(acceleration);
    }
}

// Enable acceleration
void Rotary_Manager::enableAcceleration() {
    accelerationEnabled = true;
    if (encoder) {
        encoder->setAcceleration(accelerationValue);
    }
}

// Disable acceleration
void Rotary_Manager::disableAcceleration() {
    accelerationEnabled = false;
    if (encoder) {
        encoder->disableAcceleration();
    }
}

// Set conservative mode (disable acceleration for precise control)
void Rotary_Manager::setConservativeMode(bool enabled) {
    if (enabled) {
        Serial.println("Rotary encoder: Conservative mode enabled (no acceleration)");
        disableAcceleration();
    } else {
        Serial.println("Rotary encoder: Conservative mode disabled (acceleration enabled)");
        enableAcceleration();
    }
}

// Set boundaries
void Rotary_Manager::setBoundaries(int16_t min_val, int16_t max_val, bool circle) {
    if (encoder) {
        encoder->setBoundaries(min_val, max_val, circle);
        boundariesSet = true;
    }
}

// Update function (call in main loop)
void Rotary_Manager::update() {
    if (!encoder) return;
    
    // Check for button click
    //if (encoder->isEncoderButtonClicked()) {
    //    Serial.println("Rotary encoder button clicked!");
    //    // Could be used for mute/unmute or other functions
    //}
    
    // Check for encoder value changes
    int16_t encoderDelta = encoder->encoderChanged();
    
    if (encoderDelta != 0) {
        // Get new encoder value
        int16_t newEncoderValue = encoder->readEncoder();
        
        // Validate the new value is within reasonable bounds
        if (newEncoderValue < 0 || newEncoderValue > 100) {
            Serial.printf("Invalid encoder value detected: %d, clamping to valid range\n", newEncoderValue);
            // Clamp to valid range and reset encoder
            newEncoderValue = constrain(newEncoderValue, 0, 100);
            encoder->setEncoderValue(newEncoderValue);
        }
        
        // Check if the change is reasonable (not a huge jump)
        int16_t change = abs(newEncoderValue - encoderValue);
        if (change > 20) { // If change is more than 20 steps, it's likely an error
            Serial.printf("Large encoder jump detected: %d -> %d (change: %d), ignoring\n", 
                        encoderValue, newEncoderValue, change);
            // Reset to last known good value
            encoder->setEncoderValue(encoderValue);
            return;
        }
        
        // Update encoder value
        encoderValue = newEncoderValue;
        
        // Calculate new volume
        float newVolume = encoderValue / 100.0f;
        
        // Update volume if changed
        if (newVolume != currentVolume) {
            currentVolume = newVolume;
            Serial.printf("Volume changed: %.2f (encoder: %d, delta: %d)\n", 
                        currentVolume, encoderValue, encoderDelta);
            
            // Call callback if set
            if (volumeChangeCallback) {
                volumeChangeCallback(currentVolume);
            }
        }
        
        lastEncoderValue = encoderValue;
    }
}

// Set volume change callback
void Rotary_Manager::setVolumeChangeCallback(void (*callback)(float volume)) {
    volumeChangeCallback = callback;
}

// Get encoder value
int16_t Rotary_Manager::getEncoderValue() const {
    return encoderValue;
}

// Check if button was clicked
bool Rotary_Manager::isButtonClicked() const {
    if (encoder) {
        return encoder->isEncoderButtonClicked();
    }
    return false;
}

// Reset encoder
void Rotary_Manager::reset() {
    if (encoder) {
        encoder->reset();
        encoderValue = (int16_t)(currentVolume * 100.0f);
        lastEncoderValue = encoderValue;
    }
}
