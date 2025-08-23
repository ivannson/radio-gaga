# DAC_Manager Class Documentation

## Overview

The `DAC_Manager` class provides a clean, configurable interface for initializing and managing the Adafruit TLV320DAC3100 I2S DAC/ADC. It encapsulates all the complex initialization and configuration logic, making it easy to integrate into your projects.

## Features

- **Configurable pin assignments** - Set custom reset, SDA, and SCL pins
- **Flexible I2C address** - Support for different DAC addresses
- **Multiple configuration levels** - Basic, full, or custom configuration
- **Runtime control** - Volume control, speaker enable/disable, headphone detection
- **Error handling** - Comprehensive error checking and reporting
- **Clean interface** - Simple API that hides the complexity of the underlying DAC library

## Files

- `include/DAC_Manager.h` - Header file with class declaration
- `src/DAC_Manager.cpp` - Implementation file
- `examples/DAC_Usage_Examples.cpp` - Usage examples
- `README_DAC_Manager.md` - This documentation

## Quick Start

### Basic Usage

```cpp
#include "DAC_Manager.h"

// Create DAC manager with default pins
DAC_Manager dac;

void setup() {
    Serial.begin(115200);
    
    // Initialize and configure DAC
    if (dac.begin()) {
        dac.configureBasic();
        Serial.println("DAC ready!");
    }
}
```

### Custom Pin Configuration

```cpp
// Create DAC manager with custom pins
DAC_Manager dac(5, 23, 22, 0x18);  // reset=5, sda=23, scl=22, address=0x18

if (dac.begin()) {
    dac.configureBasic();
}
```

### Full Configuration

```cpp
DAC_Manager dac;

if (dac.begin()) {
    dac.configure(
        true,   // enable headphone detection
        true,   // enable speaker output
        8,      // headphone volume (0-15)
        2       // speaker volume (0-15)
    );
}
```

## API Reference

### Constructor

```cpp
DAC_Manager(uint8_t reset_pin = 4, uint8_t sda_pin = 22, uint8_t scl_pin = 21, uint8_t address = 0x18)
```

**Parameters:**
- `reset_pin` - GPIO pin for DAC reset (default: 4)
- `sda_pin` - I2C SDA pin (default: 22)
- `scl_pin` - I2C SCL pin (default: 21)
- `address` - I2C address of the DAC (default: 0x18)

### Initialization

```cpp
bool begin()
```

Initializes the DAC, including I2C setup, reset sequence, and basic communication test.

**Returns:** `true` if successful, `false` if failed

### Configuration Methods

#### Basic Configuration
```cpp
bool configureBasic()
```

Performs minimal DAC configuration for basic I2S operation.

#### Full Configuration
```cpp
bool configureFull()
```

Performs complete DAC configuration including headphone detection, speaker output, and volume control.

#### Custom Configuration
```cpp
bool configure(bool enable_headphone_detection = true, 
               bool enable_speaker_output = true,
               uint8_t headphone_volume = 6,
               uint8_t speaker_volume = 0)
```

Configures the DAC with custom settings.

### Runtime Control

#### Volume Control
```cpp
void setHeadphoneVolume(uint8_t volume)  // volume: 0-15
void setSpeakerVolume(uint8_t volume)    // volume: 0-15
```

#### Speaker Control
```cpp
void enableSpeaker(bool enable)
```

#### Headphone Detection
```cpp
tlv320_headset_status_t getHeadphoneStatus()
```

Returns:
- `TLV320_HEADSET_NONE` - No headphones connected
- `TLV320_HEADSET_WITHOUT_MIC` - Headphones connected (no microphone)
- `TLV320_HEADSET_WITH_MIC` - Headset connected (with microphone)

### Status and Access

#### Status Check
```cpp
bool isInitialized() const
```

Returns `true` if the DAC is properly initialized.

#### Direct Codec Access
```cpp
Adafruit_TLV320DAC3100* getCodec()
```

Returns a pointer to the underlying DAC codec object for advanced operations.

### Utility Methods

#### I2C Communication Test
```cpp
bool checkI2CCommunication()
```

Tests if the DAC is responding on the I2C bus.

#### Reset DAC
```cpp
void reset()
```

Performs a hardware reset of the DAC.

## Error Handling

The DAC_Manager includes comprehensive error handling:

1. **I2C Communication Errors** - Checks if DAC responds on the bus
2. **Initialization Errors** - Verifies DAC library initialization
3. **Configuration Errors** - Validates each configuration step
4. **Status Validation** - Ensures DAC is ready before operations

## Pin Mapping

### Default Configuration
- **Reset Pin**: GPIO 4
- **I2C SDA**: GPIO 22
- **I2C SCL**: GPIO 21
- **I2C Address**: 0x18

### Important Notes
- **GPIO 1** should be avoided as it's used for UART0 TX
- **GPIO 2** is used for SD card data line
- Choose pins that don't conflict with other hardware

## Integration with Main Project

The DAC_Manager is designed to integrate seamlessly with the main audio player project:

```cpp
// In main.cpp
#include "DAC_Manager.h"

// Create DAC manager instance
DAC_Manager dacManager(4, 22, 21, 0x18);

void setup() {
    // ... other initialization code ...
    
    // Initialize DAC
    if (!dacManager.begin()) {
        // Handle initialization error
        return;
    }
    
    // Configure DAC
    if (!dacManager.configureBasic()) {
        // Handle configuration error
        return;
    }
    
    // ... continue with rest of setup ...
}
```

## Troubleshooting

### Common Issues

1. **"DAC not responding on I2C bus"**
   - Check wiring connections (SDA/SCL)
   - Verify DAC is powered
   - Confirm I2C address is correct

2. **"Failed to initialize TLV320DAC3100"**
   - Check reset pin connection
   - Verify I2C communication is working
   - Ensure DAC library is properly installed

3. **"DAC configuration failed"**
   - Check if DAC is properly initialized
   - Verify all required pins are available
   - Ensure no pin conflicts with other hardware

### Debug Output

The DAC_Manager provides detailed debug output during initialization and configuration. Monitor the serial output to identify where issues occur.

## Examples

See `examples/DAC_Usage_Examples.cpp` for comprehensive usage examples covering:
- Basic setup
- Custom pin configuration
- Full configuration
- Runtime control
- Error handling
- Advanced usage 