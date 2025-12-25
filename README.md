# ESP32 RFID Audio Player

I saw Yoto audiobook player for kids which costs over £75 and then you need a subscription or pay £7+ per book so I decided to spend a lot more money (and time) developing my own! This is still work in progress, checkout TO DOs to see what's still to be completed, but this is at a stage of what seems to be a fully working prototype. I'll share the scehmatic when I finish it + create Bill of Materials. 

This ESP32-based audio player system that uses RFID cards to control music playback from SD card storage. Features include setup mode for RFID-to-folder mapping, headphone detection, volume control, and comprehensive logging.

✨ Check out the [build progress on Instagram](https://www.instagram.com/nvkv.makes/) and stay tuned for the full build video [coming soon on YouTube](https://www.youtube.com/@Nvkvmakes)! 🎥

## 🎵 Features

### Core Functionality
- **RFID-Controlled Playback**: Present RFID cards to automatically start playing music from mapped folders
- **SD Card Audio Storage**: Supports MP3 files stored on SD card with automatic file discovery
- **Headphone Detection**: Automatic audio routing between speakers and headphones
- **Volume Control**: Rotary encoder for precise volume adjustment
- **Button Controls**: Play/pause, next/previous track, and setup mode access

### Setup & Configuration
- **Setup Mode**: Semi-automated RFID-to-folder mapping system
- **Settings File**: `settings.json` is automatically created for storing WiFi credentials, default volume etc
- **Visual Feedback**: LED indicators for system status
- **Mapping Storage**: Persistent storage of RFID UID to audio folder mappings
- **Folder Scanning**: Automatic discovery of audio folders on SD card

### Advanced Features
- **Dual Audio Modes**: Built-in folder-based playback and custom file list management
- **Battery Monitoring**: Optional battery level monitoring with fuel gauge
- **Settings Management**: Persistent configuration storage
- **Comprehensive Logging**: Structured logging system with multiple levels

## 🏗️ Hardware Requirements

### Core Components
- **ESP32 Development Board** (ESP32 Wroom, Wrover or similar)
- **MFRC522 RFID Reader Module** (SPI interface)
- **SD Card Module** (SD_MMC interface)
- **Adafruit TLV320DAC3100 Audio DAC** (I2C interface)
- **Rotary Encoder** (for volume control)
- **Push Buttons** (play/pause, next, previous, encoder button)
- **WS2812B LED** (status indicator)

### Pin Configuration
```
RFID Module (MFRC522):
- SCLK: GPIO 18
- MISO: GPIO 19  
- MOSI: GPIO 23
- SS: GPIO 5
- RST: GPIO 4

SD Card (SD_MMC):
- CLK: GPIO 14
- CMD: GPIO 15
- D0: GPIO 2

Audio DAC (TLV320DAC3100):
- SDA: GPIO 21
- SCL: GPIO 22
- RESET: GPIO 16

I2S Audio Output:
- BCK: GPIO 26
- WS: GPIO 25
- DATA: GPIO 32

Headphone Detection:
- MIC (detect): GPIO 33 (with pull-up)
- OFF (shutdown): GPIO 35 ⚠️ Input-only - needs swap with MIC for auto-off feature

Rotary Encoder:
- CLK: GPIO 12
- DT: GPIO 13
- SW: GPIO 14
- VCC: GPIO 15

Buttons (ADC-based):
- ADC Pin: GPIO 34

LED:
- WS2812B: GPIO 27
```

## 📁 Project Structure

```
├── include/                 # Header files
│   ├── Audio_Manager.h     # Audio playback management
│   ├── Battery_Manager.h   # Battery monitoring
│   ├── Button_Manager.h    # Button input handling
│   ├── DAC_Manager.h       # Audio DAC control
│   ├── Logger.h            # Logging system
│   ├── MappingStore.h      # RFID mapping storage
│   ├── RFID_Manager.h      # RFID card handling
│   ├── Rotary_Manager.h    # Volume control
│   ├── SD_Manager.h        # SD card management
│   ├── SD_Scanner.h        # Folder scanning
│   ├── SetupMode.h         # Setup mode state machine
│   └── Settings_Manager.h  # Configuration management
├── src/                    # Source files
│   ├── Audio_Manager.cpp   # Audio playback implementation
│   ├── Battery_Manager.cpp # Battery monitoring
│   ├── Button_Manager.cpp  # Button handling
│   ├── DAC_Manager.cpp     # DAC control
│   ├── Logger.cpp          # Logging implementation
│   ├── MappingStore.cpp    # Mapping storage
│   ├── RFID_Manager.cpp    # RFID handling
│   ├── Rotary_Manager.cpp  # Volume control
│   ├── SD_Manager.cpp      # SD card management
│   ├── SD_Scanner.cpp      # Folder scanning
│   ├── SetupMode.cpp       # Setup mode implementation
│   ├── Settings_Manager.cpp# Configuration management
│   └── main.cpp            # Main application
├── platformio.ini          # PlatformIO configuration
└── README.md               # This file
```

## 🚀 Getting Started

### Prerequisites
- [PlatformIO](https://platformio.org/) installed
- ESP32 development board
- Required hardware components (see Hardware Requirements)


### Initial Setup
1. **Prepare SD Card**: Create folders with MP3 files on your SD card (`settings.json` is created automatically)
2. **Enter Setup Mode**: Long-press the encoder button to enter setup mode
3. **Map RFID Cards**: Follow the setup prompts to map RFID cards to audio folders
4. **Test Playback**: Present mapped RFID cards to start audio playback

## 🎛️ Usage

### Normal Operation
- **Play Music**: Present a mapped RFID card to start playback
- **Control Playback**: Use buttons for play/pause, next/previous track
- **Adjust Volume**: Turn the rotary encoder to change volume
- **Headphone Detection**: Audio automatically routes to headphones when connected

### Setup Mode
- **Enter Setup**: Long-press encoder button
- **Map Cards**: Follow prompts to assign RFID cards to audio folders (currently in serial monitor, needs to be changed for wireless or audio communication)
- **Navigate**: Use play/pause button to advance through setup steps
- **Exit Setup**: Complete mapping or press encoder button again

### LED Indicators
- **Green**: System ready and operational
- **Red**: Error state or initialization failure
- **Red Flash (3x)**: Unknown RFID card presented

## 📊 Logging System

The project includes a logging system with four levels:

### Log Levels
- **ERROR**: Critical failures, initialization errors, system crashes
- **WARN**: Non-critical issues, fallback behaviors, missing components
- **INFO**: Important system events, initialization success, user actions
- **DEBUG**: Detailed debugging information, file listings, state changes

### Component-Specific Logging
Each component has its own logging prefix:
- `[AUDIO]` - Audio playback and file management
- `[RFID]` - RFID card detection and handling
- `[SETUP]` - Setup mode operations
- `[BUTTON]` - Button input processing
- `[DAC]` - Audio DAC operations
- `[SD]` - SD card operations
- `[BATTERY]` - Battery monitoring
- `[SETTINGS]` - Configuration management
- `[ROTARY]` - Volume control
- `[MAPPING]` - RFID mapping operations

### Runtime Control
Log levels can be controlled at runtime:
```cpp
initLogger(LogLevel::INFO);  // Set initial log level
setLogLevel(LogLevel::DEBUG); // Change log level during runtime
```

## 🔧 Configuration

### Audio Settings
- **Default Volume**: set in `settings.json`
- **File Extensions**: .mp3 (others not implemented/tested)
- **Buffer Size**: 1024 samples
- **Buffer Count**: 4 buffers

### RFID Settings
- **Self-Test**: Enabled by default
- **Debounce**: 50ms
- **Audio Control**: Enabled (can be disabled during setup)

### Button Settings
- **ADC Resolution**: 12-bit
- **Voltage Thresholds**: Configurable per button
- **Debounce**: 50ms

## 🐛 Troubleshooting

### Common Issues

**No Audio Output**
- Check I2S pin connections
- Verify DAC initialization in logs
- Ensure audio files are in correct format

**RFID Not Detected**
- Check SPI connections
- Verify MFRC522 initialization
- Check RFID card compatibility

**SD Card Not Mounted**
- Verify SD_MMC connections
- Check SD card format (FAT32)
- Ensure SD card is properly inserted

**Setup Mode Issues**
- Check button functionality
- Verify LED connections
- Review setup mode logs

### Debug Information
Enable DEBUG logging to get detailed system information:
```cpp
setLogLevel(LogLevel::DEBUG);
```

## 📚 Dependencies

### Core Libraries
- **ESP32 Arduino Core**: ESP32 development framework
- **MFRC522**: RFID card reader library
- **SD_MMC**: SD card interface
- **Wire**: I2C communication
- **SPI**: SPI communication
- **FastLED**: WS2812B LED control

### Audio Libraries
- **Audio**: Audio playback and I2S
- **AudioTools**: Audio processing utilities

### Additional Libraries
- **MAX1704x**: Battery fuel gauge
- **AiEsp32RotaryEncoder**: Rotary encoder handling

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 📋 TODO

### Hardware Documentation
- [ ] Create schematic diagram
- [ ] Design PCB layout
- [ ] Generate Bill of Materials (BOM)

### Configuration & Settings
- [ ] Add logging level configuration to `settings.json`

### Power Management
- [ ] Implement auto-shutdown timer (configurable via settings.json)
  - ⚠️ **Hardware limitation**: Cannot be implemented with current pinout. The MIC pin is on GPIO33 and the OFF pin is on GPIO35. Since GPIO35 is input-only, it cannot send the shutdown signal. These pins need to be swapped: MIC detection (input-only) on GPIO35, and OFF signal (output) on GPIO33.

### Visual Feedback Improvements
- [ ] Improve/add on to LED feedback system
- [ ] Implement battery level LED indicator (green → orange → red)

### Wireless Features
- [ ] Implement WiFi connectivity
- [ ] Create web server for remote monitoring
- [ ] Add runtime log level selection via web interface
- [ ] Develop web-based setup mode interface
- [ ] Implement wireless file upload via browser
- [ ] Add option to switch between serial and WiFi monitoring

## 📞 Support

For issues and questions:
1. Check the troubleshooting section
2. Review the logs with DEBUG level enabled
3. Create an issue with detailed information
4. Include relevant log output and hardware configuration

---

**Happy Listening! 🎵**
