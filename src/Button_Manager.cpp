#include "Button_Manager.h"

// Constructor
Button_Manager::Button_Manager(uint8_t adc_pin, float encoder_voltage, float previous_voltage, 
                               float play_pause_voltage, float next_voltage) {
    adcPin = adc_pin;
    adcResolution = 12;  // 12-bit ADC
    
    // Set voltage thresholds
    encoderButtonVoltage = encoder_voltage;
    previousButtonVoltage = previous_voltage;
    playPauseButtonVoltage = play_pause_voltage;
    nextButtonVoltage = next_voltage;
    
    // Default tolerance
    voltageTolerance = 0.1f;  // 100mV tolerance
    
    // Initialize state
    currentButton = BUTTON_NONE;
    lastButton = BUTTON_NONE;
    buttonState = BUTTON_RELEASED;
    
    // Initialize timing
    lastPressTime = 0;
    lastReleaseTime = 0;
    holdTime = 0;
    debounceTime = 0;
    
    // Default timing thresholds
    holdThreshold = 200;        // 500ms for hold
    longPressThreshold = 2000;  // 2 seconds for long press
    debounceThreshold = 50;     // 50ms debounce
}

// Initialize the button manager
bool Button_Manager::begin() {
    Serial.println("Initializing Button Manager...");
    Serial.printf("ADC Pin: %d\n", adcPin);
    Serial.printf("Voltage thresholds:\n");
    Serial.printf("  Encoder: %.2fV\n", encoderButtonVoltage);
    Serial.printf("  Previous: %.2fV\n", previousButtonVoltage);
    Serial.printf("  Play/Pause: %.2fV\n", playPauseButtonVoltage);
    Serial.printf("  Next: %.2fV\n", nextButtonVoltage);
    Serial.printf("  Tolerance: %.2fV\n", voltageTolerance);
    
    // Configure ADC
    analogReadResolution(adcResolution);
    // Note: ESP32 Arduino framework handles attenuation automatically
    // No need to set attenuation manually
    
    Serial.println("Button Manager initialized successfully!");
    return true;
}

// Update button state (call this regularly in loop)
void Button_Manager::update() {
    unsigned long currentTime = millis();
    float voltage = getVoltage();
    //Serial.printf("ADC Value: %d, Voltage: %.3fV\n", getRawADC(), voltage);
    
    // Determine which button is pressed based on voltage
    ButtonType detectedButton = BUTTON_NONE;
    
    if (voltage > 0.1f) {  // Any button pressed (voltage > 0.1V)
        if (abs(voltage - encoderButtonVoltage) <= voltageTolerance) {
            detectedButton = BUTTON_ENCODER;
        } else if (abs(voltage - previousButtonVoltage) <= voltageTolerance) {
            detectedButton = BUTTON_PREVIOUS;
        } else if (abs(voltage - playPauseButtonVoltage) <= voltageTolerance) {
            detectedButton = BUTTON_PLAY_PAUSE;
        } else if (abs(voltage - nextButtonVoltage) <= voltageTolerance) {
            detectedButton = BUTTON_NEXT;
        }
    }
    
    // Handle button state changes
    if (detectedButton != currentButton) {
        // Button state changed
        if (detectedButton == BUTTON_NONE) {
            // Button released
            if (currentTime - lastPressTime > longPressThreshold) {
                buttonState = BUTTON_RELEASED_LONG;
            } else {
                buttonState = BUTTON_RELEASED;
            }
            lastReleaseTime = currentTime;
            lastButton = currentButton;
            currentButton = BUTTON_NONE;
            
            // Print button release
            if (lastButton != BUTTON_NONE) {
                Serial.printf("Button released: %s\n", getButtonName(lastButton));
            }
        } else {
            // Button pressed
            if (currentTime - lastReleaseTime > debounceThreshold) {
                currentButton = detectedButton;
                buttonState = BUTTON_PRESSED;
                lastPressTime = currentTime;
                
                // Print button press
                Serial.printf("Button pressed: %s (%.2fV)\n", getButtonName(currentButton), voltage);
            }
        }
    } else if (detectedButton != BUTTON_NONE) {
        // Button is held
        if (currentTime - lastPressTime > holdThreshold) {
            buttonState = BUTTON_HELD;
            holdTime = currentTime - lastPressTime;
        }
    }
}

// Check if specific button is pressed
bool Button_Manager::isButtonPressed(ButtonType button) const {
    return (currentButton == button && buttonState == BUTTON_PRESSED);
}

// Check if specific button is held
bool Button_Manager::isButtonHeld(ButtonType button) const {
    return (currentButton == button && buttonState == BUTTON_HELD);
}

// Check if specific button is released
bool Button_Manager::isButtonReleased(ButtonType button) const {
    return (lastButton == button && buttonState == BUTTON_RELEASED);
}

// Get button name as string
const char* Button_Manager::getButtonName(ButtonType button) const {
    switch (button) {
        case BUTTON_NONE: return "NONE";
        case BUTTON_ENCODER: return "ENCODER";
        case BUTTON_PREVIOUS: return "PREVIOUS";
        case BUTTON_PLAY_PAUSE: return "PLAY/PAUSE";
        case BUTTON_NEXT: return "NEXT";
        default: return "UNKNOWN";
    }
}

// Get raw ADC value
uint16_t Button_Manager::getRawADC() const {
    return analogRead(adcPin);
}

// Get voltage reading
float Button_Manager::getVoltage() const {
    uint16_t adcValue = analogRead(adcPin);
    // Convert ADC value to voltage (assuming 3.3V reference)
    return (adcValue * 3.3f) / ((1 << adcResolution) - 1);
}

// Print debug information
void Button_Manager::printDebugInfo() {
    uint16_t rawADC = getRawADC();
    float voltage = getVoltage();
    
    Serial.printf("ADC Debug - Raw: %d, Voltage: %.3fV, Button: %s, State: %d\n", 
                  rawADC, voltage, getButtonName(currentButton), buttonState);
}

// Calibrate voltage thresholds
void Button_Manager::calibrate() {
    Serial.println("Button calibration mode - press each button:");
    Serial.println("1. Encoder button");
    Serial.println("2. Previous button");
    Serial.println("3. Play/Pause button");
    Serial.println("4. Next button");
    Serial.println("Press any button to start calibration...");
    
    // Wait for any button press to start
    while (getVoltage() < 0.1f) {
        delay(10);
    }
    
    Serial.println("Calibration started. Press each button one by one:");
    
    // Calibrate each button
    const char* buttonNames[] = {"Encoder", "Previous", "Play/Pause", "Next"};
    float* thresholds[] = {&encoderButtonVoltage, &previousButtonVoltage, 
                          &playPauseButtonVoltage, &nextButtonVoltage};
    
    for (int i = 0; i < 4; i++) {
        Serial.printf("Press %s button...\n", buttonNames[i]);
        
        // Wait for button press
        while (getVoltage() < 0.1f) {
            delay(10);
        }
        
        // Read voltage
        float voltage = getVoltage();
        *thresholds[i] = voltage;
        
        Serial.printf("%s button voltage: %.3fV\n", buttonNames[i], voltage);
        
        // Wait for button release
        while (getVoltage() > 0.1f) {
            delay(10);
        }
        
        delay(500);  // Wait between buttons
    }
    
    Serial.println("Calibration complete!");
    Serial.printf("New thresholds:\n");
    Serial.printf("  Encoder: %.3fV\n", encoderButtonVoltage);
    Serial.printf("  Previous: %.3fV\n", previousButtonVoltage);
    Serial.printf("  Play/Pause: %.3fV\n", playPauseButtonVoltage);
    Serial.printf("  Next: %.3fV\n", nextButtonVoltage);
} 