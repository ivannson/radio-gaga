#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <AudioTools.h>
#include "AudioTools/Disk/AudioSourceSDMMC.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "SD_MMC.h"

// File selection mode enum
enum class FileSelectionMode {
    BUILTIN,    // Use AudioPlayer's built-in next/previous methods
    CUSTOM      // Use custom file list iteration with playPath()
};

class Audio_Manager {
private:
    // Audio pipeline components
    AudioSourceSDMMC* source;
    I2SStream* i2s;
    VolumeStream* volume;
    MP3DecoderHelix* decoder;
    AudioPlayer* player;
    I2SConfig i2sCfg_;
    
    // Configuration
    const char* audioFolder;
    const char* fileExtension;
    float currentVolume;
    bool audioInitialized;
    bool playerActive;
    
    // File management
    String currentFile;
    String firstAudioFile;
    bool filesListed;
    bool filesAvailable;
    
    // File selection mode
    FileSelectionMode fileSelectionMode;
    
    // Custom file list for CUSTOM mode
    std::vector<String> audioFileList;
    int currentFileIndex;
    
    // I2S configuration
    uint8_t i2sBckPin;
    uint8_t i2sWsPin;
    uint8_t i2sDataPin;
    uint8_t i2sChannels;
    uint8_t i2sBitsPerSample;
    uint16_t i2sBufferSize;
    uint8_t i2sBufferCount;
    
    
    // Audio folder path
    static constexpr const char* kDefaultAudioFolder = "/test_audio";
    static constexpr const char* kDefaultExtension = "mp3";
    static constexpr float kDefaultVolume = 0.35f;
    static constexpr uint16_t kDefaultBufferSize = 1024;
    static constexpr uint8_t kDefaultBufferCount = 8;
    static constexpr FileSelectionMode kDefaultFileSelectionMode = FileSelectionMode::BUILTIN;

public:
    // Constructor
    Audio_Manager(const char* folder = kDefaultAudioFolder, 
                  const char* ext = kDefaultExtension,
                  FileSelectionMode mode = kDefaultFileSelectionMode);
    
    // Destructor
    ~Audio_Manager();
    
    // Initialization
    bool begin();
    bool initializeI2S();
    bool initializeAudioPipeline();
    
    // File operations
    bool listAudioFiles();
    bool playFile(const String& filename);
    bool playNextFile();
    bool playPreviousFile();
    bool stopPlayback();
    bool pausePlayback();
    bool resumePlayback();
    bool restartFromFirstFile();
    
    // Playback control
    bool isPlaying() const;
    bool isPaused() const;
    bool isStopped() const;
    bool isPlaybackHealthy() const;
    void setVolume(float volume);
    float getVolume() const;
    
    // File information
    String getCurrentFile() const;
    String getFirstAudioFile() const;
    bool areFilesAvailable() const;
    int getFileCount() const;
    
    // Audio pipeline management
    void update(); // Call in main loop
    void updatePlaybackState(); // Check and update playback state
    bool isInitialized() const { return audioInitialized; }
    
    // Configuration
    void setI2SPins(uint8_t bck, uint8_t ws, uint8_t data);
    void setBufferSettings(uint16_t bufferSize, uint8_t bufferCount);
    void setAudioFolder(const char* folder);
    void setFileExtension(const char* ext);
    void setFileSelectionMode(FileSelectionMode mode);
    FileSelectionMode getFileSelectionMode() const { return fileSelectionMode; }
    
    // Dynamic audio source management
    bool changeAudioSource(const char* newFolder);
    
    // Debug and status
    void printAudioStatus() const;
    void printFileList() const;
    const char* getLastError() const;

private:
    // Internal helper functions
    bool findFirstAudioFile();
    bool validateAudioFile(const String& filename);
    void sendSilence(int frames);
    void clearAudioPipeline();
    
    // Custom file selection helpers
    bool buildCustomFileList();
    bool playFileByIndex(int index);
    int findFileIndex(const String& filename);
    
    // Error handling
    void setLastError(const char* error) const; // Make const-correct
    mutable char lastError[128]; // Make mutable so it can be modified in const functions
    
    // File counting
    int totalAudioFiles;
};

#endif // AUDIO_MANAGER_H
