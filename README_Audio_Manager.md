# Audio_Manager

A C++ class for managing audio playback from SD card using arduino-audio-tools in the Smart Audio Player project.

## Features

- **SD Card Audio**: Plays MP3 files from SD card folders
- **I2S Output**: Configurable I2S pins for external DAC
- **Volume Control**: Integrated volume management
- **File Management**: Lists and manages audio files
- **Playback Control**: Play, stop, pause, resume functionality
- **Error Handling**: Comprehensive error reporting and recovery
- **Buffer Management**: ESP32-optimized buffer settings

## Audio Pipeline

The Audio_Manager creates a complete audio pipeline:

```
SD Card → AudioSourceSDMMC → MP3DecoderHelix → VolumeStream → I2SStream → DAC
```

## Pin Configuration

- **BCK Pin**: GPIO 26 (I2S Bit Clock)
- **WS Pin**: GPIO 25 (I2S Word Select)
- **DATA Pin**: GPIO 32 (I2S Data)

## Usage

### Basic Setup

```cpp
#include "Audio_Manager.h"

// Create instance (defaults to /test_audio folder, mp3 files)
Audio_Manager audioManager("/test_audio", "mp3");

// Initialize
if (!audioManager.begin()) {
    Serial.printf("Failed to initialize: %s\n", audioManager.getLastError());
    return;
}
```

### Audio Playback

```cpp
// Play a specific file
if (audioManager.playFile("song1.mp3")) {
    Serial.println("Playback started!");
} else {
    Serial.printf("Failed to play: %s\n", audioManager.getLastError());
}

// Check playback status
if (audioManager.isPlaying()) {
    Serial.println("Audio is playing");
}

// Stop playback
audioManager.stopPlayback();
```

### Volume Control

```cpp
// Set volume (0.0 to 1.0)
audioManager.setVolume(0.5f); // 50%

// Get current volume
float volume = audioManager.getVolume();
```

### File Management

```cpp
// List all audio files
audioManager.listAudioFiles();

// Get file information
int fileCount = audioManager.getFileCount();
String firstFile = audioManager.getFirstAudioFile();
bool filesAvailable = audioManager.areFilesAvailable();
```

### Main Loop Integration

```cpp
void loop() {
    // Update audio manager (required for playback)
    audioManager.update();
    
    // Your other code here
    delay(10);
}
```

## Configuration

### I2S Pins

```cpp
// Custom I2S pin configuration
audioManager.setI2SPins(26, 25, 32); // BCK, WS, DATA
```

### Buffer Settings

```cpp
// Custom buffer configuration for ESP32
audioManager.setBufferSettings(1024, 8); // buffer size, count
```

### Audio Folder

```cpp
// Change audio source folder
audioManager.setAudioFolder("/music");
audioManager.setFileExtension("wav");
```

## File Structure

The Audio_Manager expects audio files in a specific folder structure:

```
SD Card Root/
├── test_audio/          # Default audio folder
│   ├── song1.mp3
│   ├── song2.mp3
│   └── song3.mp3
├── settings.json        # Settings file
└── other files...
```

## Audio Formats

Currently supports:
- **MP3**: Using MP3DecoderHelix (recommended)
- **Future**: WAV, FLAC, OGG support planned

## Error Handling

```cpp
// Check initialization status
if (!audioManager.isInitialized()) {
    Serial.printf("Audio error: %s\n", audioManager.getLastError());
    return;
}

// Handle playback errors
if (!audioManager.playFile("song.mp3")) {
    Serial.printf("Playback failed: %s\n", audioManager.getLastError());
}
```

## Performance

### Buffer Settings
- **Default Buffer Size**: 1024 bytes (ESP32 optimized)
- **Default Buffer Count**: 8 buffers
- **Memory Usage**: ~8KB for buffers

### Audio Quality
- **Sample Rate**: 44.1kHz (MP3 standard)
- **Bit Depth**: 16-bit
- **Channels**: Stereo (2 channels)

## Dependencies

- **arduino-audio-tools**: Audio processing library
- **SD_MMC**: SD card file system
- **Arduino Framework**: ESP32 Arduino core

## Integration with Other Managers

### Settings_Manager
```cpp
// Load volume from settings
float defaultVolume = settingsManager.getDefaultVolume();
audioManager.setVolume(defaultVolume);
```

### Rotary_Manager
```cpp
// Volume control via rotary encoder
void onVolumeChanged(float newVolume) {
    audioManager.setVolume(newVolume);
}
rotaryManager.setVolumeChangeCallback(onVolumeChanged);
```

### DAC_Manager
```cpp
// Audio output goes through I2S to DAC
// DAC_Manager handles headphone detection and routing
```

## Troubleshooting

### Common Issues

1. **No Audio Output**
   - Check I2S pin connections
   - Verify DAC is powered and configured
   - Check buffer settings

2. **Audio Stuttering**
   - Increase buffer size
   - Reduce buffer count
   - Check SD card speed

3. **File Not Found**
   - Verify folder path exists
   - Check file extension
   - Ensure SD card is mounted

### Debug Information

```cpp
// Print detailed status
audioManager.printAudioStatus();
audioManager.printFileList();

// Check error messages
const char* error = audioManager.getLastError();
Serial.printf("Last error: %s\n", error);
```

## Future Enhancements

- **Playlist Support**: Sequential file playback
- **Audio Effects**: Equalizer, reverb, etc.
- **Streaming**: WiFi audio streaming
- **Format Support**: Additional audio codecs
- **Metadata**: ID3 tag reading
- **Remote Control**: WiFi-based playback control
