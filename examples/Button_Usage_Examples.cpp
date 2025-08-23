/*
 * Button_Manager Usage Examples
 * 
 * This file shows different ways to configure and use the Button_Manager class
 */

#include "Button_Manager.h"

// Example 1: Basic setup with default voltage thresholds
void example1_basicSetup() {
    // Create button manager with default settings
    Button_Manager buttons;
    
    // Initialize
    if (buttons.begin()) {
        Serial.println("Button Manager initialized!");
        
        // In your main loop:
        while (true) {
            buttons.update();
            delay(10);
        }
    }
}

// Example 2: Custom voltage thresholds
void example2_customThresholds() {
    // Create button manager with custom voltage thresholds
    Button_Manager buttons(39, 0.60f, 1.00f, 1.60f, 2.00f);  // adc_pin, encoder, previous, play_pause, next
    
    if (buttons.begin()) {
        Serial.println("Button Manager with custom thresholds initialized!");
    }
}

// Example 3: Button detection
void example3_buttonDetection() {
    Button_Manager buttons;
    
    if (buttons.begin()) {
        while (true) {
            buttons.update();
            
            // Check for specific button presses
            if (buttons.isButtonPressed(BUTTON_ENCODER)) {
                Serial.println("Encoder button pressed!");
            }
            
            if (buttons.isButtonPressed(BUTTON_PREVIOUS)) {
                Serial.println("Previous button pressed!");
            }
            
            if (buttons.isButtonPressed(BUTTON_PLAY_PAUSE)) {
                Serial.println("Play/Pause button pressed!");
            }
            
            if (buttons.isButtonPressed(BUTTON_NEXT)) {
                Serial.println("Next button pressed!");
            }
            
            delay(10);
        }
    }
}

// Example 4: Button hold detection
void example4_buttonHold() {
    Button_Manager buttons;
    
    if (buttons.begin()) {
        while (true) {
            buttons.update();
            
            // Check for button holds
            if (buttons.isButtonHeld(BUTTON_ENCODER)) {
                Serial.println("Encoder button held!");
            }
            
            if (buttons.isButtonHeld(BUTTON_PLAY_PAUSE)) {
                Serial.println("Play/Pause button held!");
            }
            
            delay(10);
        }
    }
}

// Example 5: Button release detection
void example5_buttonRelease() {
    Button_Manager buttons;
    
    if (buttons.begin()) {
        while (true) {
            buttons.update();
            
            // Check for button releases
            if (buttons.isButtonReleased(BUTTON_ENCODER)) {
                Serial.println("Encoder button released!");
            }
            
            if (buttons.isButtonReleased(BUTTON_PREVIOUS)) {
                Serial.println("Previous button released!");
            }
            
            delay(10);
        }
    }
}

// Example 6: Get current button state
void example6_currentState() {
    Button_Manager buttons;
    
    if (buttons.begin()) {
        while (true) {
            buttons.update();
            
            // Get current button and state
            ButtonType currentButton = buttons.getCurrentButton();
            ButtonState buttonState = buttons.getButtonState();
            
            if (currentButton != BUTTON_NONE) {
                Serial.printf("Current button: %s, State: %d\n", 
                             buttons.getButtonName(currentButton), buttonState);
            }
            
            delay(10);
        }
    }
}

// Example 7: ADC debugging
void example7_adcDebug() {
    Button_Manager buttons;
    
    if (buttons.begin()) {
        while (true) {
            buttons.update();
            
            // Print debug information every second
            static unsigned long lastDebug = 0;
            if (millis() - lastDebug > 1000) {
                buttons.printDebugInfo();
                lastDebug = millis();
            }
            
            delay(10);
        }
    }
}

// Example 8: Custom timing thresholds
void example8_customTiming() {
    Button_Manager buttons;
    
    if (buttons.begin()) {
        // Set custom timing thresholds
        buttons.setHoldThreshold(300);        // 300ms for hold
        buttons.setLongPressThreshold(3000);  // 3 seconds for long press
        buttons.setDebounceThreshold(100);    // 100ms debounce
        
        Serial.println("Custom timing thresholds set!");
        
        while (true) {
            buttons.update();
            delay(10);
        }
    }
}

// Example 9: Custom voltage tolerance
void example9_voltageTolerance() {
    Button_Manager buttons;
    
    if (buttons.begin()) {
        // Set custom voltage tolerance
        buttons.setVoltageTolerance(0.05f);  // 50mV tolerance
        
        Serial.println("Custom voltage tolerance set!");
        
        while (true) {
            buttons.update();
            delay(10);
        }
    }
}

// Example 10: Button calibration
void example10_calibration() {
    Button_Manager buttons;
    
    if (buttons.begin()) {
        Serial.println("Starting button calibration...");
        
        // Run calibration
        buttons.calibrate();
        
        Serial.println("Calibration complete! Now testing buttons...");
        
        // Test calibrated buttons
        while (true) {
            buttons.update();
            delay(10);
        }
    }
}

// Example 11: Audio player control simulation
void example11_audioControl() {
    Button_Manager buttons;
    
    if (buttons.begin()) {
        bool isPlaying = false;
        
        while (true) {
            buttons.update();
            
            // Simulate audio player controls
            if (buttons.isButtonPressed(BUTTON_PLAY_PAUSE)) {
                isPlaying = !isPlaying;
                Serial.printf("Play/Pause: %s\n", isPlaying ? "Playing" : "Paused");
            }
            
            if (buttons.isButtonPressed(BUTTON_PREVIOUS)) {
                Serial.println("Previous track");
            }
            
            if (buttons.isButtonPressed(BUTTON_NEXT)) {
                Serial.println("Next track");
            }
            
            if (buttons.isButtonPressed(BUTTON_ENCODER)) {
                Serial.println("Encoder button - could be used for volume or menu");
            }
            
            delay(10);
        }
    }
}

// Example 12: Integration with other managers
void example12_integration() {
    Button_Manager buttons;
    // DAC_Manager dac;  // Uncomment if you have DAC_Manager
    
    if (buttons.begin()) {
        Serial.println("Button Manager ready for integration!");
        
        while (true) {
            buttons.update();
            
            // Example integration with audio controls
            if (buttons.isButtonPressed(BUTTON_PLAY_PAUSE)) {
                // dac.playPause();  // Example DAC control
                Serial.println("Play/Pause command sent to DAC");
            }
            
            if (buttons.isButtonPressed(BUTTON_PREVIOUS)) {
                // dac.previousTrack();  // Example DAC control
                Serial.println("Previous track command sent to DAC");
            }
            
            if (buttons.isButtonPressed(BUTTON_NEXT)) {
                // dac.nextTrack();  // Example DAC control
                Serial.println("Next track command sent to DAC");
            }
            
            delay(10);
        }
    }
} 