# Rotary_Manager

A C++ class for managing rotary encoder input for volume control in the Smart Audio Player project.

## Features

- **Volume Control**: Maps encoder values (0-100) to volume range (0.0-1.0)
- **Acceleration Support**: Built-in acceleration for faster volume changes
- **Interrupt-Driven**: Uses ESP32 interrupts for responsive input
- **Callback System**: Volume change notifications via callback functions
- **Configurable**: Customizable boundaries, acceleration, and volume ranges
- **Button Support**: Handles encoder button clicks

## Pin Configuration

- **CLK Pin**: GPIO 27 (Encoder A pin)
- **DT Pin**: GPIO 34 (Encoder B pin)  
- **Button Pin**: GPIO 39 (Shared with other buttons via ADC)
- **VCC Pin**: -1 (Connected directly to 3.3V)

## Usage

### Basic Setup

```cpp
#include "Rotary_Manager.h"

// Create instance
Rotary_Manager rotaryManager(27, 34, 39);

// Initialize
if (!rotaryManager.begin()) {
    Serial.println("Failed to initialize!");
    return;
}

// Set volume change callback
rotaryManager.setVolumeChangeCallback(onVolumeChanged);
```

### Volume Control

```cpp
// Set initial volume (0.0 to 1.0)
rotaryManager.setVolume(0.2f); // 20%

// Get current volume
float volume = rotaryManager.getVolume();

// Set custom volume range
rotaryManager.setVolumeRange(0.1f, 0.8f); // 10% to 80%
```

### Configuration

```cpp
// Set acceleration (0 = disabled, higher = faster)
rotaryManager.setAcceleration(250);

// Disable acceleration for fine control
rotaryManager.disableAcceleration();

// Enable conservative mode to prevent encoder skipping
rotaryManager.setConservativeMode(true);

// Set encoder boundaries
rotaryManager.setBoundaries(0, 100, false); // 0-100, no wrap-around
```

### Main Loop Integration

```cpp
void loop() {
    // Update encoder state
    rotaryManager.update();
    
    // Check for button clicks
    if (rotaryManager.isButtonClicked()) {
        // Handle button press
    }
    
    delay(10);
}
```

## Volume Change Callback

```cpp
void onVolumeChanged(float newVolume) {
    Serial.printf("Volume: %.2f\n", newVolume);
    
    // Apply to audio system
    audioPlayer.setVolume(newVolume);
    dacManager.setVolume(newVolume);
}
```

## Technical Details

### Encoder Resolution
- Uses 4 steps per detent for smooth operation
- Maps 0-100 encoder values to 0.0-1.0 volume range
- Supports acceleration for faster volume changes
- **Anti-skip protection**: Detects and prevents large value jumps
- **Conservative mode**: Option to disable acceleration for precise control

### Interrupt Handling
- Uses ESP32 interrupt service routine (ISR)
- Non-blocking operation
- Responsive to rapid encoder turns

### Memory Management
- Dynamic allocation of encoder instance
- Proper cleanup in destructor
- Static instance pointer for ISR access

## Dependencies

- **AiEsp32RotaryEncoder**: [GitHub Repository](https://github.com/igorantolic/ai-esp32-rotary-encoder)
- **Arduino Framework**: ESP32 Arduino core

## Error Handling

- Initialization failures return `false` from `begin()`
- Serial output for debugging
- Graceful degradation if encoder is not available

## Future Enhancements

- **Settings Integration**: Load default volume from settings.json
- **Audio Integration**: Direct volume control of audio pipeline
- **LED Feedback**: Visual volume indication via WLED
- **Preset Volumes**: Quick access to common volume levels
