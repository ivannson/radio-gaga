# Smart Audio Player - Complete Pseudocode

## System Architecture

### Hardware Components
- ESP32 Wrover (dual-core)
- SD card (SDMMC one-bit mode) with audio files organized in folders
- RFID-RC522 module for tag detection
- Adafruit TLV320DAC3100 external DAC/amplifier with headphone detection
- Rotary encoder for volume control
- 4 buttons: 1 independent on/off switch, 3 playback controls + encoder button on single ADC pin
- Single WLED for battery level indication
- MAX17043G+U I2C fuel gauge for battery monitoring

### Pin Mapping
```
23 - rfid miso
22 - dac sda; fuel gauge sda
1 - dac reset
21 - dac scl
19 - rfid mosi
18 - rfid sclk
5 - rfid chip select
4 - sd mmc DO
15 - sd mmc DI
36 - sd mmc card detect (CON)
39 - 4 adc buttons (0.55V, 0.97V, 1.54V, 1.94V)
34 - encoder dt
35 - on/off
32 - dac DIN
25 - dac WSEL
26 - dac BCK
27 - encoder CLK
14 - sd mmc sclk
13 - wled
```

### Button Voltage Thresholds (ADC Pin 39)
- **Encoder Button**: 0.55V
- **Previous Button**: 0.97V  
- **Play/Pause Button**: 1.54V
- **Next Button**: 1.94V
- **Tolerance**: ±0.1V (configurable)
- **Debounce Time**: 50ms (configurable)
- **Hold Threshold**: 200ms (configurable)

## Core Functionality

### Manager Classes (Implemented)
- **SD_Manager**: Handles SD card operations, file listing, and management
- **DAC_Manager**: Manages TLV320DAC3100 initialization and configuration
- **Button_Manager**: Handles 4-button ADC detection with voltage thresholds
- **Rotary_Manager**: Controls volume via rotary encoder with acceleration and settings integration
- **Settings_Manager**: Manages JSON configuration files with backup/restore capabilities
- **Audio_Manager**: Handles audio pipeline with SD card source, MP3 decoder, and I2S output

### Dual-Core Operation
- **Core 0**: RFID detection and tag management
- **Core 1**: Audio processing and user interface

### RFID Logic
- Track last 5 readings
- Debounce tag detection (3 readings)
- Same tag present → no action
- Tag removed for >3 readings → pause playback
- Same tag re-presented → continue playback
- Different tag → play new content

### Audio Routing
- Headphones when connected, speaker when not
- Automatic switching with pause on disconnect
- Continue playback when headphones connected

## Detailed Pseudocode

### Initialization Sequence
```
1. Initialize SD Card (SDMMC one-bit mode)
   - Enable pull-ups for pins 2,4,12,13,15,14
   - Mount SD card
   - Verify card type and access

2. Load/Create Settings (settings.json)
   - Check if file exists
   - If not: create with defaults (volume=0.1, etc.)
   - If exists: load into memory structure
   - Validate JSON format

3. Load Tag Lookup Table (tags.csv)
   - Parse CSV format: tag_uid,folder_name
   - Store in vector<TagEntry>
   - Validate all entries

4. Initialize Hardware
   - I2C setup (SDA=22, SCL=21)
   - TLV320DAC3100 configuration
   - MAX17043G+U fuel gauge setup
   - RFID-RC522 SPI setup
   - GPIO pins (buttons, LED, encoder)
   - SD MMC pins

5. Initialize Audio Pipeline
   - I2S stream configuration
   - Volume stream setup
   - MP3 decoder initialization
   - AudioPlayer creation
   - Set initial volume from settings

6. Create Dual-Core Tasks
   - RFID task on Core 0
   - Audio task on Core 1
   - Inter-core communication queue
   - Mutex for audio access
```

### Core 0: RFID Task
```cpp
void rfidTask(void* parameter) {
    while(true) {
        // Check for RFID cards every 200ms
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
            bool currentlyDetected = true;
            tagAbsentCounter = 0;
            
            // Check if this is the same tag
            bool isSameTag = compareUID(mfrc522.uid.uidByte, lastDetectedUID, mfrc522.uid.size);
            
            if (!tagPresent || !isSameTag) {
                // New or different tag
                tagPresentCounter++;
                
                if (tagPresentCounter >= DEBOUNCE_COUNT) {
                    // Debounced new tag detection
                    processRFIDTag(mfrc522.uid.uidByte, mfrc522.uid.size);
                    
                    // Store tag details
                    memcpy(lastDetectedUID, mfrc522.uid.uidByte, mfrc522.uid.size);
                    lastDetectedUIDSize = mfrc522.uid.size;
                    tagPresent = true;
                    tagPresentCounter = 0;
                }
            } else {
                // Same tag still present
                tagPresentCounter = 0;
            }
        } else {
            // No tag detected
            if (tagPresent) {
                tagPresentCounter = 0;
                tagAbsentCounter++;
                
                if (tagAbsentCounter >= DEBOUNCE_COUNT) {
                    // Tag is confirmed absent
                    tagPresent = false;
                    tagAbsentCounter = 0;
                    
                    // Send pause command
                    CommandMessage msg = {CMD_PAUSE, ""};
                    xQueueSend(commandQueue, &msg, 0);
                }
            }
        }
        
        // Check headphones every 1000ms
        checkHeadphoneStatus();
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```

### Core 1: Audio Task
```cpp
void audioTask(void* parameter) {
    while (true) {
        // Process commands from RFID task
        CommandMessage msg;
        if (xQueueReceive(commandQueue, &msg, pdMS_TO_TICKS(10)) == pdTRUE) {
            handleAudioCommand(msg.cmd, msg.data);
        }
        
        // Copy audio data if playing
        if (playState == PLAYING && player->isActive()) {
            player->copy();
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
```

### Button Handling
```cpp
void handleButtons() {
    float voltage = readButtonVoltage();
    
    // Determine which button based on voltage levels
    int buttonIndex = -1;
    if (voltage < 0.7) {        // 0.55V - Encoder Button
        buttonIndex = 0;
    } else if (voltage < 1.2) { // 0.97V - Previous
        buttonIndex = 1;
    } else if (voltage < 1.7) { // 1.54V - Play/Pause
        buttonIndex = 2;
    } else if (voltage < 2.2) { // 1.94V - Next
        buttonIndex = 3;
    }
    
    // Handle button state changes with debouncing
    for (int i = 0; i < 4; i++) {
        bool buttonPressed = (i == buttonIndex);
        
        if (buttonPressed && !buttonStates[i]) {
            // Button just pressed
            if (currentTime - buttonPressTimes[i] > BUTTON_DEBOUNCE_TIME) {
                buttonStates[i] = true;
                buttonPressTimes[i] = currentTime;
                
                // Handle the button press
                switch (i) {
                    case 0: handleEncoderButton(); break;
                    case 1: handlePreviousButton(); break;
                    case 2: handlePlayPauseButton(); break;
                    case 3: handleNextButton(); break;
                }
            }
        }
    }
}
```

### Audio Processing Functions
```cpp
void loadAudioSource(const char* folderPath) {
    if (xSemaphoreTake(audioMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Stop current playback
        if (player->isActive()) {
            player->stop();
        }
        
        // Clean up old source
        if (source != nullptr) {
            delete source;
        }
        
        // Create new source
        source = new AudioSourceSDMMC(folderPath, "mp3");
        player->setAudioSource(*source);
        
        // Start playback
        player->begin();
        playState = PLAYING;
        currentFolder = String(folderPath);
        
        xSemaphoreGive(audioMutex);
    }
}

const char* findFolderForUID(const char* uid) {
    for (const auto& entry : tagLookupTable) {
        if (strcmp(entry.uid, uid) == 0) {
            return entry.folder;
        }
    }
    return nullptr;
}
```

### Headphone Detection
```cpp
void checkHeadphoneStatus() {
    tlv320_headset_status_t status = codec.getHeadsetStatus();
    
    if (status != lastHeadphoneStatus) {
        switch (status) {
            case TLV320_HEADSET_NONE:
                // Headphones disconnected
                codec.enableSpeaker(true);
                if (playState == PLAYING) {
                    handleAudioCommand(CMD_PAUSE);
                }
                break;
                
            case TLV320_HEADSET_WITHOUT_MIC:
            case TLV320_HEADSET_WITH_MIC:
                // Headphones connected
                codec.enableSpeaker(false);
                break;
        }
        
        lastHeadphoneStatus = status;
    }
}
```

### Battery and LED Functions
```cpp
void setBatteryLED(int level) {
    if (level >= 50) {
        // Green (single flash)
        digitalWrite(WLED_PIN, HIGH);
        delay(100);
        digitalWrite(WLED_PIN, LOW);
    } else if (level >= 25) {
        // Orange (double flash)
        for (int i = 0; i < 2; i++) {
            digitalWrite(WLED_PIN, HIGH);
            delay(100);
            digitalWrite(WLED_PIN, LOW);
            delay(100);
        }
    } else {
        // Red (triple flash)
        for (int i = 0; i < 3; i++) {
            digitalWrite(WLED_PIN, HIGH);
            delay(100);
            digitalWrite(WLED_PIN, LOW);
            delay(100);
        }
    }
}

void setErrorLED() {
    // Non-blocking red flashing for runtime errors
    static unsigned long lastErrorFlash = 0;
    static bool errorLedState = false;
    
    unsigned long currentTime = millis();
    if (currentTime - lastErrorFlash > 500) {
        errorLedState = !errorLedState;
        digitalWrite(WLED_PIN, errorLedState ? HIGH : LOW);
        lastErrorFlash = currentTime;
    }
}
```

### Settings Structure
```cpp
struct Settings {
    float defaultVolume = 0.1f;
    char wifiSSID[32] = "";
    char wifiPassword[64] = "";
    int sleepTimeout = 15; // minutes
    int batteryCheckInterval = 1; // minutes
};
```

### Tag Lookup Structure
```cpp
struct TagEntry {
    char uid[32];
    char folder[64];
};
std::vector<TagEntry> tagLookupTable;
```

### Inter-Core Communication
```cpp
enum AudioCommand { 
    CMD_NONE, 
    CMD_PLAY_NEW, 
    CMD_PAUSE, 
    CMD_RESUME, 
    CMD_STOP,
    CMD_VOLUME_UP,
    CMD_VOLUME_DOWN,
    CMD_NEXT_TRACK,
    CMD_PREV_TRACK
};

struct CommandMessage {
    AudioCommand cmd;
    char data[64]; // For folder names, etc.
};
```

## Implementation Order
1. ✅ LED functionality (GPIO 13)
2. ✅ SD card initialization (SD_Manager)
3. ✅ DAC/headphone detection (DAC_Manager)
4. ✅ Button handling (ADC on GPIO 39) - Button_Manager
5. ✅ Rotary encoder (GPIO 34, 27) - Rotary_Manager
6. ✅ Settings management (JSON) - Settings_Manager
7. 🔄 Tag lookup table (CSV)
8. 🔄 RFID functionality
9. ✅ Audio pipeline setup - Audio_Manager
10. 🔄 Battery monitoring
11. 🔄 Dual-core tasks
12. 🔄 Power management

## Error Handling
- Red flashing LED for all errors initially
- Critical errors (startup failures) use blocking error function
- Runtime errors use non-blocking error function
- Future: implement error codes and remote logging 