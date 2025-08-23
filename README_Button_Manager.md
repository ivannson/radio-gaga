# Button_Manager Class Documentation

## Overview

The `Button_Manager` class provides a clean, configurable interface for detecting multiple buttons connected to a single ADC pin using voltage dividers. It's designed for the ESP32's ADC and supports configurable voltage thresholds, debouncing, and multiple button states.

## Features

- **Multi-button detection** - Up to 4 buttons on a single ADC pin
- **Configurable voltage thresholds** - Set custom voltage levels for each button
- **Debouncing** - Built-in debounce protection
- **Multiple button states** - Press, hold, release, and long press detection
- **Auto-calibration** - Built-in calibration mode for voltage thresholds
- **Debug output** - Comprehensive debugging and monitoring
- **Clean interface** - Simple API that hides ADC complexity

## Files

- `include/Button_Manager.h` - Header file with class declaration
- `src/Button_Manager.cpp` - Implementation file
- `examples/Button_Usage_Examples.cpp` - Usage examples
- `README_Button_Manager.md` - This documentation

## Quick Start

### Basic Usage

```cpp
#include "Button_Manager.h"

// Create button manager with default voltage thresholds
Button_Manager buttons;

void setup() {
    Serial.begin(115200);
    
    // Initialize button manager
    if (buttons.begin()) {
        Serial.println("Button Manager ready!");
    }
}

void loop() {
    // Update button states
    buttons.update();
    
    // Check for button presses
    if (buttons.isButtonPressed(BUTTON_PLAY_PAUSE)) {
        Serial.println("Play/Pause button pressed!");
    }
    
    delay(10);
}
```

### Custom Voltage Thresholds

```cpp
// Create button manager with custom voltage thresholds
Button_Manager buttons(39, 0.60f, 1.00f, 1.60f, 2.00f);  // adc_pin, encoder, previous, play_pause, next

if (buttons.begin()) {
    Serial.println("Button Manager with custom thresholds initialized!");
}
```

## API Reference

### Constructor

```cpp
Button_Manager(uint8_t adc_pin = 39, 
               float encoder_voltage = 0.55f,
               float previous_voltage = 0.97f,
               float play_pause_voltage = 1.54f,
               float next_voltage = 1.94f)
```

**Parameters:**
- `adc_pin` - ADC pin number (default: 39)
- `encoder_voltage` - Voltage threshold for encoder button (default: 0.55V)
- `previous_voltage` - Voltage threshold for previous button (default: 0.97V)
- `play_pause_voltage` - Voltage threshold for play/pause button (default: 1.54V)
- `next_voltage` - Voltage threshold for next button (default: 1.94V)

### Initialization

```cpp
bool begin()
```

Initializes the button manager, configures ADC, and prints configuration.

**Returns:** `true` if successful, `false` if failed

### Button State Management

#### Update Button States
```cpp
void update()
```

Updates button states. Call this regularly in your main loop.

#### Get Current State
```cpp
ButtonType getCurrentButton() const
ButtonType getLastButton() const
ButtonState getButtonState() const
```

Returns the current button, last button, and button state.

### Button Detection

#### Check Specific Buttons
```cpp
bool isButtonPressed(ButtonType button) const
bool isButtonHeld(ButtonType button) const
bool isButtonReleased(ButtonType button) const
```

Check if a specific button is pressed, held, or released.

#### Button Types
```cpp
enum ButtonType {
    BUTTON_NONE = 0,
    BUTTON_ENCODER,
    BUTTON_PREVIOUS,
    BUTTON_PLAY_PAUSE,
    BUTTON_NEXT
}
```

#### Button States
```cpp
enum ButtonState {
    BUTTON_RELEASED = 0,
    BUTTON_PRESSED,
    BUTTON_HELD,
    BUTTON_RELEASED_LONG
}
```

### Utility Methods

#### Get Button Name
```cpp
const char* getButtonName(ButtonType button) const
```

Returns the button name as a string.

#### ADC Information
```cpp
uint16_t getRawADC() const
float getVoltage() const
```

Get raw ADC value and converted voltage.

#### Debug Information
```cpp
void printDebugInfo()
```

Prints current ADC and button state information.

### Configuration

#### Timing Thresholds
```cpp
void setHoldThreshold(unsigned long threshold)      // Default: 500ms
void setLongPressThreshold(unsigned long threshold) // Default: 2000ms
void setDebounceThreshold(unsigned long threshold)  // Default: 50ms
```

Set custom timing thresholds for button behavior.

#### Voltage Tolerance
```cpp
void setVoltageTolerance(float tolerance)  // Default: 0.1V
```

Set voltage tolerance for button detection.

### Calibration

```cpp
void calibrate()
```

Runs interactive calibration mode to set voltage thresholds automatically.

## Pin Mapping

### Default Configuration
- **ADC Pin**: GPIO 39 (ADC1_CH3)
- **Voltage Range**: 0-3.3V
- **ADC Resolution**: 12-bit
- **Attenuation**: 11dB (0-3.3V range)

### Button Voltage Thresholds
- **Encoder Button**: 0.55V
- **Previous Button**: 0.97V
- **Play/Pause Button**: 1.54V
- **Next Button**: 1.94V

### Hardware Setup
```
VCC (3.3V) ----[R1]----+----[R2]----[Button]----GND
                        |
                        +---- ADC Pin
```

Where R1 and R2 form voltage dividers for each button.

## Integration with Main Project

The Button_Manager integrates seamlessly with the main audio player project:

```cpp
// In main.cpp
#include "Button_Manager.h"

// Create button manager instance
Button_Manager buttonManager(ADC_BUTTONS_PIN, 0.55f, 0.97f, 1.54f, 1.94f);

void setup() {
    // ... other initialization code ...
    
    // Initialize button manager
    if (!buttonManager.begin()) {
        // Handle initialization error
        return;
    }
    
    // ... continue with rest of setup ...
}

void loop() {
    // Update button states
    buttonManager.update();
    
    // Handle button presses
    if (buttonManager.isButtonPressed(BUTTON_PLAY_PAUSE)) {
        // Handle play/pause
    }
    
    // ... rest of loop code ...
}
```

## Error Handling

The Button_Manager includes comprehensive error handling:

1. **ADC Configuration Errors** - Validates ADC setup
2. **Voltage Reading Errors** - Handles out-of-range readings
3. **Button State Errors** - Manages invalid button states
4. **Timing Errors** - Handles millis() overflow

## Common Use Cases

### Audio Player Controls
```cpp
Button_Manager buttons;

if (buttons.begin()) {
    while (true) {
        buttons.update();
        
        if (buttons.isButtonPressed(BUTTON_PLAY_PAUSE)) {
            // Toggle play/pause
            togglePlayback();
        }
        
        if (buttons.isButtonPressed(BUTTON_PREVIOUS)) {
            // Previous track
            playPreviousTrack();
        }
        
        if (buttons.isButtonPressed(BUTTON_NEXT)) {
            // Next track
            playNextTrack();
        }
        
        if (buttons.isButtonPressed(BUTTON_ENCODER)) {
            // Volume control or menu
            handleEncoderButton();
        }
        
        delay(10);
    }
}
```

### Button Hold Actions
```cpp
Button_Manager buttons;

if (buttons.begin()) {
    while (true) {
        buttons.update();
        
        // Long press for power off
        if (buttons.isButtonHeld(BUTTON_PLAY_PAUSE)) {
            Serial.println("Long press detected - powering off...");
            // Power off sequence
        }
        
        delay(10);
    }
}
```

### Calibration Mode
```cpp
Button_Manager buttons;

if (buttons.begin()) {
    // Run calibration if needed
    if (needCalibration) {
        buttons.calibrate();
    }
    
    // Normal operation
    while (true) {
        buttons.update();
        delay(10);
    }
}
```

## Troubleshooting

### Common Issues

1. **"Button not detected"**
   - Check voltage divider resistors
   - Verify voltage thresholds
   - Run calibration mode
   - Check ADC pin connection

2. **"False button presses"**
   - Increase voltage tolerance
   - Check for electrical noise
   - Verify resistor values
   - Add hardware debouncing

3. **"Inconsistent readings"**
   - Check power supply stability
   - Verify ADC configuration
   - Check for loose connections
   - Run calibration

4. **"Button stuck"**
   - Check button hardware
   - Verify voltage divider
   - Check for short circuits
   - Reset button manager

### Debug Output

The Button_Manager provides detailed debug output:

```
Initializing Button Manager...
ADC Pin: 39
Voltage thresholds:
  Encoder: 0.55V
  Previous: 0.97V
  Play/Pause: 1.54V
  Next: 1.94V
  Tolerance: 0.10V
Button Manager initialized successfully!
Button pressed: PLAY/PAUSE (1.54V)
Button released: PLAY/PAUSE
```

### Performance Considerations

- **Update frequency** - Call `update()` every 10-50ms for responsive buttons
- **ADC resolution** - 12-bit provides good voltage precision
- **Voltage tolerance** - 100mV default tolerance balances accuracy and reliability
- **Debounce time** - 50ms default prevents false triggers

## Examples

See `examples/Button_Usage_Examples.cpp` for comprehensive usage examples covering:
- Basic setup
- Custom thresholds
- Button detection
- Hold detection
- Release detection
- Current state monitoring
- ADC debugging
- Custom timing
- Voltage tolerance
- Calibration
- Audio player control
- Integration with other managers 