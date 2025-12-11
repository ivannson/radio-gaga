#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

// Button types
enum ButtonType {
    BUTTON_NONE = 0,
    BUTTON_ENCODER,
    BUTTON_PREVIOUS,
    BUTTON_PLAY_PAUSE,
    BUTTON_NEXT
};

// Button state
enum ButtonState {
    BUTTON_RELEASED = 0,
    BUTTON_PRESSED,
    BUTTON_HELD,
    BUTTON_RELEASED_LONG
};

class Button_Manager {
private:
    // ADC configuration
    uint8_t adcPin;
    uint8_t adcResolution;
    
    // Voltage thresholds (in volts)
    float encoderButtonVoltage;
    float previousButtonVoltage;
    float playPauseButtonVoltage;
    float nextButtonVoltage;
    
    // Tolerance for voltage reading (in volts)
    float voltageTolerance;
    
    // Button state tracking
    ButtonType currentButton;
    ButtonType lastButton;
    ButtonState buttonState;
    
    // Last raw detected button (before debouncing)
    ButtonType lastDetectedButton;
    
    // Press event tracking - prevents multiple press events during long press
    bool pressEventRegistered;
    
    // Timing
    unsigned long lastPressTime;
    unsigned long lastReleaseTime;
    unsigned long holdTime;
    unsigned long debounceTime;
    
    // Configuration
    unsigned long holdThreshold;
    unsigned long longPressThreshold;
    unsigned long debounceThreshold;

public:
    // Constructor with default ADC pin and voltage thresholds
    Button_Manager(uint8_t adc_pin = 39, 
                   float encoder_voltage = 0.55f,
                   float previous_voltage = 0.97f,
                   float play_pause_voltage = 1.54f,
                   float next_voltage = 1.94f);
    
    // Initialize the button manager
    bool begin();
    
    // Update button state (call this regularly in loop)
    void update();
    
    // Get current button state
    ButtonType getCurrentButton() const { return currentButton; }
    ButtonType getLastButton() const { return lastButton; }
    ButtonState getButtonState() const { return buttonState; }
    
    // Check if specific button is pressed
    bool isButtonPressed(ButtonType button) const;
    bool isButtonHeld(ButtonType button) const;
    bool isButtonReleased(ButtonType button) const;
    
    // Get current press duration (in milliseconds)
    // Returns 0 if no button is currently pressed
    unsigned long getPressDuration() const;
    
    // Get button name as string
    const char* getButtonName(ButtonType button) const;
    
    // Get raw ADC value
    uint16_t getRawADC() const;
    
    // Get voltage reading
    float getVoltage() const;
    
    // Configure timing thresholds
    void setHoldThreshold(unsigned long threshold) { holdThreshold = threshold; }
    void setLongPressThreshold(unsigned long threshold) { longPressThreshold = threshold; }
    void setDebounceThreshold(unsigned long threshold) { debounceThreshold = threshold; }
    
    // Configure voltage tolerance
    void setVoltageTolerance(float tolerance) { voltageTolerance = tolerance; }
    
    // Print debug information
    void printDebugInfo();
    
    // Calibrate voltage thresholds
    void calibrate();
};

#endif // BUTTON_MANAGER_H 