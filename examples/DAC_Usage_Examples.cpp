/*
 * DAC_Manager Usage Examples
 * 
 * This file shows different ways to configure and use the DAC_Manager class
 */

#include "DAC_Manager.h"

// Example 1: Basic setup with default pins
void example1_basicSetup() {
    // Create DAC manager with default pins (reset=4, sda=22, scl=21, address=0x18)
    DAC_Manager dac;
    
    // Initialize and configure with basic settings
    if (dac.begin()) {
        dac.configureBasic();
        Serial.println("DAC initialized with basic configuration");
    }
}

// Example 2: Custom pin configuration
void example2_customPins() {
    // Create DAC manager with custom pins
    DAC_Manager dac(5, 23, 22, 0x18);  // reset=5, sda=23, scl=22, address=0x18
    
    if (dac.begin()) {
        dac.configureBasic();
        Serial.println("DAC initialized with custom pins");
    }
}

// Example 3: Full configuration with custom settings
void example3_fullConfiguration() {
    DAC_Manager dac;
    
    if (dac.begin()) {
        // Configure with custom settings
        dac.configure(
            true,   // enable headphone detection
            true,   // enable speaker output
            8,      // headphone volume (0-15)
            2       // speaker volume (0-15)
        );
        Serial.println("DAC initialized with full configuration");
    }
}

// Example 4: Step-by-step configuration
void example4_stepByStep() {
    DAC_Manager dac;
    
    if (dac.begin()) {
        // Do basic configuration first
        dac.configureBasic();
        
        // Then add additional features as needed
        // (This would require adding more methods to DAC_Manager)
        Serial.println("DAC initialized step by step");
    }
}

// Example 5: Runtime volume control
void example5_runtimeControl() {
    DAC_Manager dac;
    
    if (dac.begin() && dac.configureBasic()) {
        // Change volumes at runtime
        dac.setHeadphoneVolume(10);  // Set headphone volume to 10
        dac.setSpeakerVolume(5);     // Set speaker volume to 5
        
        // Enable/disable speaker
        dac.enableSpeaker(false);    // Disable speaker
        
        // Check headphone status
        tlv320_headset_status_t status = dac.getHeadphoneStatus();
        switch(status) {
            case TLV320_HEADSET_NONE:
                Serial.println("No headphones connected");
                dac.enableSpeaker(true);  // Enable speaker
                break;
            case TLV320_HEADSET_WITHOUT_MIC:
                Serial.println("Headphones connected (no mic)");
                dac.enableSpeaker(false); // Disable speaker
                break;
            case TLV320_HEADSET_WITH_MIC:
                Serial.println("Headset connected (with mic)");
                dac.enableSpeaker(false); // Disable speaker
                break;
        }
    }
}

// Example 6: Advanced usage with direct codec access
void example6_advancedUsage() {
    DAC_Manager dac;
    
    if (dac.begin() && dac.configureBasic()) {
        // Get direct access to the codec for advanced operations
        Adafruit_TLV320DAC3100* codec = dac.getCodec();
        
        // Use the codec directly for advanced configurations
        // (This gives you access to all the original library functions)
        
        Serial.println("DAC ready for advanced operations");
    }
}

// Example 7: Error handling
void example7_errorHandling() {
    DAC_Manager dac(4, 22, 21, 0x18);
    
    // Check if DAC is responding
    if (!dac.begin()) {
        Serial.println("DAC initialization failed!");
        return;
    }
    
    // Check if configuration succeeded
    if (!dac.configureBasic()) {
        Serial.println("DAC configuration failed!");
        return;
    }
    
    // Check if DAC is ready for use
    if (!dac.isInitialized()) {
        Serial.println("DAC not properly initialized!");
        return;
    }
    
    Serial.println("DAC ready for use!");
} 