/*
 * Rotary_Manager Usage Examples
 * 
 * This file demonstrates how to use the Rotary_Manager class
 * for volume control with a rotary encoder.
 */

#include "Rotary_Manager.h"

// Pin definitions for ESP32 Wrover
#define ROTARY_CLK_PIN 27    // Encoder CLK pin
#define ROTARY_DT_PIN 34     // Encoder DT pin
#define ROTARY_BTN_PIN 39    // Encoder button (shared with other buttons via ADC)

// Create rotary manager instance
Rotary_Manager rotaryManager(ROTARY_CLK_PIN, ROTARY_DT_PIN, ROTARY_BTN_PIN);

// Volume change callback function
void onVolumeChanged(float newVolume) {
    Serial.printf("Volume callback: %.2f\n", newVolume);
    
    // Here you would apply the volume to your audio system
    // For example:
    // audioPlayer.setVolume(newVolume);
    // dacManager.setVolume(newVolume);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Rotary Encoder Volume Control Test");
    
    // Initialize rotary encoder
    if (!rotaryManager.begin()) {
        Serial.println("Failed to initialize rotary encoder!");
        while(1) delay(1000);
    }
    
    // Set volume change callback
    rotaryManager.setVolumeChangeCallback(onVolumeChanged);
    
    // Set initial volume (e.g., from settings)
    float defaultVolume = 0.2f; // 20%
    rotaryManager.setVolume(defaultVolume);
    
    // Configure acceleration (optional)
    rotaryManager.setAcceleration(250); // Medium acceleration
    
    // Enable conservative mode to prevent encoder skipping
    rotaryManager.setConservativeMode(true);
    
    Serial.println("Rotary encoder ready! Turn to change volume.");
    Serial.printf("Initial volume: %.2f\n", rotaryManager.getVolume());
    Serial.printf("Acceleration: %d (enabled: %s)\n", 
                rotaryManager.getAcceleration(), 
                rotaryManager.isAccelerationEnabled() ? "Yes" : "No");
}

void loop() {
    // Update rotary encoder (call this in your main loop)
    rotaryManager.update();
    
    // Check for button clicks
    if (rotaryManager.isButtonClicked()) {
        Serial.println("Encoder button pressed!");
        // Could be used for mute/unmute
    }
    
    // Small delay for responsiveness
    delay(10);
}

/*
 * Advanced Usage Examples:
 * 
 * // Set custom volume range
 * rotaryManager.setVolumeRange(0.1f, 0.8f); // 10% to 80%
 * 
 * // Disable acceleration for fine control
 * rotaryManager.disableAcceleration();
 * 
 * // Get current encoder value (0-100)
 * int16_t encoderVal = rotaryManager.getEncoderValue();
 * 
 * // Reset encoder to current volume
 * rotaryManager.reset();
 * 
 * // Set volume programmatically
 * rotaryManager.setVolume(0.5f); // 50%
 */
