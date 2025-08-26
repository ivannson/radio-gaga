#include "Audio_Manager.h"
#include "Logger.h"

// Define static constexpr members
constexpr const char* Audio_Manager::kDefaultAudioFolder;
constexpr const char* Audio_Manager::kDefaultExtension;
constexpr float Audio_Manager::kDefaultVolume;
constexpr uint16_t Audio_Manager::kDefaultBufferSize;
constexpr uint8_t Audio_Manager::kDefaultBufferCount;

// Constructor
Audio_Manager::Audio_Manager(const char* folder, const char* ext, FileSelectionMode mode)
    : source(nullptr), i2s(nullptr), volume(nullptr), decoder(nullptr), player(nullptr),
      audioFolder(folder), fileExtension(ext), currentVolume(kDefaultVolume),
      audioInitialized(false), playerActive(false), filesListed(false), filesAvailable(false),
      fileSelectionMode(mode), currentFileIndex(0),
      i2sBckPin(26), i2sWsPin(25), i2sDataPin(32), i2sChannels(2), i2sBitsPerSample(16),
      i2sBufferSize(kDefaultBufferSize), i2sBufferCount(kDefaultBufferCount),
      totalAudioFiles(0) {
    
    // Initialize error buffer
    strcpy(lastError, "No error");
}

// Destructor
Audio_Manager::~Audio_Manager() {
    // Clean up audio pipeline
    if (player) {
        player->stop();
        delete player;
    }
    if (volume) delete volume;
    if (decoder) delete decoder;
    if (source) delete source;
    if (i2s) delete i2s;
}

// Initialize audio manager
bool Audio_Manager::begin() {
    LOG_AUDIO_INFO("Initializing Audio Manager...");
    LOG_AUDIO_INFO("Audio folder: %s", audioFolder);
    LOG_AUDIO_INFO("File extension: %s", fileExtension);
    
    // Initialize I2S
    if (!initializeI2S()) {
        LOG_AUDIO_ERROR("Failed to initialize I2S: %s", getLastError());
        return false;
    }
    
    // Initialize audio pipeline
    if (!initializeAudioPipeline()) {
        LOG_AUDIO_ERROR("Failed to initialize audio pipeline: %s", getLastError());
        return false;
    }
    
    // List audio files
    if (!listAudioFiles()) {
        LOG_AUDIO_ERROR("Failed to list audio files: %s", getLastError());
        return false;
    }
    
    audioInitialized = true;
    LOG_AUDIO_INFO("Audio Manager initialized successfully!");
    return true;
}

// Initialize I2S
bool Audio_Manager::initializeI2S() {
    LOG_AUDIO_DEBUG("Initializing I2S...");

    i2s = new I2SStream();

    // Build the real config ONCE and keep it
    i2sCfg_ = i2s->defaultConfig(TX_MODE);
    i2sCfg_.pin_bck        = i2sBckPin;        // 26
    i2sCfg_.pin_ws         = i2sWsPin;         // 25
    i2sCfg_.pin_data       = i2sDataPin;       // 32
    i2sCfg_.channels       = i2sChannels;      // 2
    i2sCfg_.bits_per_sample= i2sBitsPerSample; // 16
    i2sCfg_.buffer_size    = i2sBufferSize;    // 1024 recommended
    i2sCfg_.buffer_count   = i2sBufferCount;   // 8   recommended

    if (!i2s->begin(i2sCfg_)) {
        setLastError("Failed to begin I2S stream");
        return false;
    }

    LOG_AUDIO_DEBUG("I2S initialized: BCK=%d, WS=%d, DATA=%d, Buffer=%d, Count=%d",
                    i2sBckPin, i2sWsPin, i2sDataPin,
                    i2sCfg_.buffer_size, i2sCfg_.buffer_count);
    return true;
}

// Initialize audio pipeline
bool Audio_Manager::initializeAudioPipeline() {
    LOG_AUDIO_DEBUG("Initializing audio pipeline...");
    
    try {
        // Create audio source
        LOG_AUDIO_DEBUG("Creating audio source for folder: %s", 
                        audioFolder[0] == '\0' ? "root directory" : audioFolder);
        
        // Handle root directory (empty string)
        const char* sourcePath = (audioFolder[0] == '\0') ? "/" : audioFolder;
        
        // Create AudioSourceSDMMC that will scan the entire folder for audio files
        // This will automatically find all files with the specified extension
        source = new AudioSourceSDMMC(sourcePath, fileExtension);
        
        LOG_AUDIO_DEBUG("Audio source created for path: %s with extension: %s", sourcePath, fileExtension);
        
        // Create volume stream
        volume = new VolumeStream(*i2s);
        auto vcfg = volume->defaultConfig();
        vcfg.copyFrom(i2sCfg_);
        if (!volume->begin(vcfg)) {
            setLastError("Failed to begin volume stream");
            return false;
        }
        
        // Create MP3 decoder
        decoder = new MP3DecoderHelix();
        
        // Create audio player
        player = new AudioPlayer(*source, *volume, *decoder);
        player->setBufferSize(i2sCfg_.buffer_size);
        
        // Set initial volume
        LOG_AUDIO_DEBUG("Setting initial volume: %.2f", currentVolume);
        volume->setVolume(currentVolume);
        
        LOG_AUDIO_DEBUG("Audio pipeline initialized successfully!");
        return true;
        
    } catch (const std::exception& e) {
        setLastError("Exception during audio pipeline initialization");
        return false;
    }
}

// List audio files in the specified folder
bool Audio_Manager::listAudioFiles() {
    LOG_AUDIO_DEBUG("=== Listing audio files in %s ===", 
                    audioFolder[0] == '\0' ? "root directory" : audioFolder);
    
    // Handle root directory (empty string)
    const char* folderPath = (audioFolder[0] == '\0') ? "/" : audioFolder;
    
    if (!SD_MMC.exists(folderPath)) {
        LOG_AUDIO_ERROR("Audio folder %s does not exist!", folderPath);
        setLastError("Audio folder not found");
        return false;
    }
    
    File folder = SD_MMC.open(folderPath);
    if (!folder || !folder.isDirectory()) {
        LOG_AUDIO_ERROR("Failed to open folder %s", folderPath);
        setLastError("Failed to open audio folder");
        return false;
    }
    
    int fileCount = 0;
    totalAudioFiles = 0;
    File file = folder.openNextFile();
    
    while (file) {
        if (!file.isDirectory()) {
            String filename = String(file.name());
            if (filename.endsWith(fileExtension)) {
                fileCount++;
                totalAudioFiles++;
                LOG_AUDIO_DEBUG("%d. %s", fileCount, filename.c_str());
                
                // Store first audio file found
                if (firstAudioFile.length() == 0) {
                    firstAudioFile = filename;
                    LOG_AUDIO_DEBUG("First audio file: %s", firstAudioFile.c_str());
                }
            }
        }
        file.close();
        file = folder.openNextFile();
    }
    
    folder.close();
    
    filesAvailable = (totalAudioFiles > 0);
    filesListed = true;
    
    LOG_AUDIO_INFO("Total audio files found: %d", totalAudioFiles);
    if (filesAvailable) {
        LOG_AUDIO_INFO("Will play first file: %s", firstAudioFile.c_str());
        
        // If in CUSTOM mode, build the custom file list
        if (fileSelectionMode == FileSelectionMode::CUSTOM) {
            if (!buildCustomFileList()) {
                LOG_AUDIO_WARN("Failed to build custom file list");
            }
        }
    } else {
        LOG_AUDIO_WARN("No audio files found!");
    }
    
    return filesAvailable;
}

// Play a specific audio file
bool Audio_Manager::playFile(const String& filename) {
    if (!audioInitialized || !filesAvailable) {
        setLastError("Audio system not ready");
        return false;
    }
    
    LOG_AUDIO_INFO("Playing file: %s (mode: %s)", filename.c_str(),
                   (fileSelectionMode == FileSelectionMode::BUILTIN) ? "BUILTIN" : "CUSTOM");
    
    // Stop current playback
    stopPlayback();
    delay(50); // Brief pause to let everything reset
    
    if (fileSelectionMode == FileSelectionMode::BUILTIN) {
        // BUILTIN mode - use the folder-based approach
        LOG_AUDIO_DEBUG("Starting playback from folder: %s", 
                        audioFolder[0] == '\0' ? "root directory" : audioFolder);
        
        // Set volume
        volume->setVolume(currentVolume);
        
        // Configure player buffer
        player->setBufferSize(i2sCfg_.buffer_size);
        
        // Start playback using AudioPlayer's begin() method
        // This will automatically start with the first file in the folder
        if (player->begin()) {
            currentFile = filename;
            playerActive = true;
            LOG_AUDIO_INFO("Started playing: %s (BUILTIN mode)", filename.c_str());
            return true;
        } else {
            setLastError("Failed to begin playback (BUILTIN mode)");
            return false;
        }
        
    } else {
        // CUSTOM mode - use playPath with full file path
        try {
            // Build full path
            String fullPath = String(audioFolder) + "/" + filename;
            LOG_AUDIO_DEBUG("Full path for custom mode: %s", fullPath.c_str());
            
            // Set volume
            volume->setVolume(currentVolume);
            
            // Configure player buffer
            player->setBufferSize(i2sCfg_.buffer_size);
            
            // Use playPath for custom mode
            if (player->playPath(fullPath.c_str())) {
                currentFile = filename;
                playerActive = true;
                
                // Update currentFileIndex for CUSTOM mode
                int fileIndex = findFileIndex(filename);
                if (fileIndex >= 0) {
                    currentFileIndex = fileIndex;
                    LOG_AUDIO_DEBUG("Updated current file index to: %d", currentFileIndex);
                }
                
                LOG_AUDIO_INFO("Started playing: %s (CUSTOM mode)", filename.c_str());
                return true;
            } else {
                setLastError("Failed to play file (CUSTOM mode)");
                return false;
            }
            
        } catch (const std::exception& e) {
            setLastError("Exception during custom file playback");
            return false;
        }
    }
}

// Stop playback
bool Audio_Manager::stopPlayback() {
    if (!player || !playerActive) {
        return true;
    }
    
    LOG_AUDIO_INFO("Stopping playback...");
    
    try {
        player->stop();
        playerActive = false;
        currentFile = "";
        
        // Send silence to clear the audio pipeline
        sendSilence(500);
        
        // Flush I2S
        if (i2s) i2s->flush();
        
        LOG_AUDIO_INFO("Playback stopped");
        return true;
        
    } catch (const std::exception& e) {
        setLastError("Exception during playback stop");
        return false;
    }
}

// Pause playback
bool Audio_Manager::pausePlayback() {
    if (!player || !playerActive) {
        return false;
    }
    
    LOG_AUDIO_INFO("Pausing playback...");
    
    // Store current file info before stopping (for potential resume)
    String pausedFile = currentFile;
    
    // Simply stop the player
    player->stop();
    playerActive = false;
    
    // Keep track of the paused file for potential resume
    if (pausedFile.length() > 0) {
        LOG_AUDIO_INFO("Playback paused on file: %s", pausedFile.c_str());
    }
    
    LOG_AUDIO_INFO("Playback paused");
    return true;
}

// Resume playback
bool Audio_Manager::resumePlayback() {
    if (!player) {
        setLastError("Player not initialized");
        return false;
    }
    
    // Check if player is actually active (not just the flag)
    if (player->isActive()) {
        LOG_AUDIO_DEBUG("Player is already active, no need to resume");
        return true;
    }
    
    // If playerActive flag is true but player is not active, reset the flag
    if (playerActive && !player->isActive()) {
        LOG_AUDIO_DEBUG("Resetting stale playerActive flag...");
        playerActive = false;
        currentFile = "";
    }
    
    // If we don't have a current file, we can't resume - need to restart
    if (currentFile.length() == 0) {
        LOG_AUDIO_DEBUG("No current file to resume from, restarting from first file...");
        setLastError("No current file, restarting from first");
        
        // Try to restart from the first file in the current folder
        if (fileSelectionMode == FileSelectionMode::BUILTIN) {
            // BUILTIN mode - restart from folder beginning
            if (filesAvailable && firstAudioFile.length() > 0) {
                LOG_AUDIO_INFO("Restarting playback from first file: %s", firstAudioFile.c_str());
                return playFile(firstAudioFile);
            } else {
                LOG_AUDIO_WARN("No first file available for restart (BUILTIN mode)");
                setLastError("No first file available for restart");
                return false;
            }
        } else {
            // CUSTOM mode - restart from first file in custom list
            if (audioFileList.size() > 0) {
                LOG_AUDIO_INFO("Restarting playback from first file in custom list: %s", audioFileList[0].c_str());
                return playFileByIndex(0);
            } else {
                LOG_AUDIO_WARN("No files in custom list for restart");
                setLastError("No files in custom list for restart");
                return false;
            }
        }
    }
    
    LOG_AUDIO_INFO("Resuming playback...");
    
    // Try to resume using AudioPlayer's simple play() method
    player->play();
    playerActive = true;
    
    // Wait a moment and check if playback actually started
    delay(100);
    
    // Check if the player is actually active and working
    bool isActive = player->isActive();
    LOG_AUDIO_DEBUG("After resume attempt: player->isActive() = %s", isActive ? "true" : "false");
    
    if (isActive) {
        LOG_AUDIO_INFO("Playback resumed successfully");
        return true;
    } else {
        LOG_AUDIO_WARN("Resume failed - player not active, attempting to restart from first file...");
        setLastError("Resume failed, restarting from first file");
        
        // Try to restart from the first file in the current folder
        if (fileSelectionMode == FileSelectionMode::BUILTIN) {
            // BUILTIN mode - restart from folder beginning
            if (filesAvailable && firstAudioFile.length() > 0) {
                LOG_AUDIO_INFO("Restarting playback from first file: %s", firstAudioFile.c_str());
                return playFile(firstAudioFile);
            } else {
                LOG_AUDIO_WARN("No first file available for restart (BUILTIN mode)");
                setLastError("No first file available for restart");
                return false;
            }
        } else {
            // CUSTOM mode - restart from first file in custom list
            if (audioFileList.size() > 0) {
                LOG_AUDIO_INFO("Restarting playback from first file in custom list: %s", audioFileList[0].c_str());
                return playFileByIndex(0);
            } else {
                LOG_AUDIO_WARN("No files in custom list for restart");
                setLastError("No files in custom list for restart");
                return false;
            }
        }
    }
}

// Check if current playback state is healthy and can be resumed
bool Audio_Manager::isPlaybackHealthy() const {
    if (!player || !audioInitialized) {
        return false;
    }
    
    // Check if player is in a good state
    if (player->isActive()) {
        return true;
    }
    
    // Check if we have files available for restart
    if (fileSelectionMode == FileSelectionMode::BUILTIN) {
        return filesAvailable && firstAudioFile.length() > 0;
    } else {
        return audioFileList.size() > 0;
    }
}

// Force restart from first file (useful when resume fails)
bool Audio_Manager::restartFromFirstFile() {
    LOG_AUDIO_INFO("Force restarting from first file...");
    
    if (fileSelectionMode == FileSelectionMode::BUILTIN) {
        // BUILTIN mode - restart from folder beginning
        if (filesAvailable && firstAudioFile.length() > 0) {
            LOG_AUDIO_INFO("Restarting playback from first file: %s", firstAudioFile.c_str());
            return playFile(firstAudioFile);
        } else {
            LOG_AUDIO_WARN("No first file available for restart (BUILTIN mode)");
            setLastError("No first file available for restart");
            return false;
        }
    } else {
        // CUSTOM mode - restart from first file in custom list
        if (audioFileList.size() > 0) {
            LOG_AUDIO_INFO("Restarting playback from first file in custom list: %s", audioFileList[0].c_str());
            return playFileByIndex(0);
        } else {
            LOG_AUDIO_WARN("No files in custom list for restart");
            setLastError("No files in custom list for restart");
            return false;
        }
    }
}

// Play next file (wraps to first when at end)
bool Audio_Manager::playNextFile() {
    if (!audioInitialized || !player) {
        setLastError("Audio system not ready");
        return false;
    }
    
    LOG_AUDIO_INFO("Moving to next file (mode: %s)...", 
                   (fileSelectionMode == FileSelectionMode::BUILTIN) ? "BUILTIN" : "CUSTOM");
    
    if (fileSelectionMode == FileSelectionMode::BUILTIN) {
        // Use AudioPlayer's built-in next() method
        if (player->next(1)) {
            playerActive = true;
            LOG_AUDIO_INFO("Next file started successfully (BUILTIN mode)");
            return true;
        } else {
            LOG_AUDIO_WARN("Failed to move to next file (BUILTIN mode), restarting from first file...");
            setLastError("Failed to move to next file (BUILTIN), restarting from first");
            
            // Try to restart from the first file
            if (filesAvailable && firstAudioFile.length() > 0) {
                LOG_AUDIO_INFO("Restarting playback from first file: %s", firstAudioFile.c_str());
                return playFile(firstAudioFile);
            } else {
                LOG_AUDIO_WARN("No first file available for restart");
                return false;
            }
        }
    } else {
        // CUSTOM mode - iterate through our file list
        if (audioFileList.size() == 0) {
            setLastError("No custom file list available");
            return false;
        }
        
        int nextIndex = (currentFileIndex + 1) % audioFileList.size();
        LOG_AUDIO_DEBUG("Moving from index %d to %d in custom list", currentFileIndex, nextIndex);
        
        if (playFileByIndex(nextIndex)) {
            LOG_AUDIO_INFO("Next file started successfully (CUSTOM mode)");
            return true;
        } else {
            LOG_AUDIO_WARN("Failed to play next file (CUSTOM mode), restarting from first...");
            setLastError("Failed to play next file (CUSTOM), restarting from first");
            
            // Try to restart from the first file in custom list
            return playFileByIndex(0);
        }
    }
}

// Play previous file (wraps to last when at beginning)
bool Audio_Manager::playPreviousFile() {
    if (!audioInitialized || !player) {
        setLastError("Audio system not ready");
        return false;
    }
    
    LOG_AUDIO_INFO("Moving to previous file (mode: %s)...", 
                   (fileSelectionMode == FileSelectionMode::BUILTIN) ? "BUILTIN" : "CUSTOM");
    
    if (fileSelectionMode == FileSelectionMode::BUILTIN) {
        // Use AudioPlayer's built-in previous() method
        if (player->previous(1)) {
            playerActive = true;
            LOG_AUDIO_INFO("Previous file started successfully (BUILTIN mode)");
            return true;
        } else {
            LOG_AUDIO_WARN("Failed to move to previous file (BUILTIN mode), restarting from first file...");
            setLastError("Failed to move to previous file (BUILTIN), restarting from first");
            
            // Try to restart from the first file
            if (filesAvailable && firstAudioFile.length() > 0) {
                LOG_AUDIO_INFO("Restarting playback from first file: %s", firstAudioFile.c_str());
                return playFile(firstAudioFile);
            } else {
                LOG_AUDIO_WARN("No first file available for restart");
                return false;
            }
        }
    } else {
        // CUSTOM mode - iterate through our file list
        if (audioFileList.size() == 0) {
            setLastError("No custom file list available");
            return false;
        }
        
        int prevIndex = (currentFileIndex - 1 + audioFileList.size()) % audioFileList.size();
        LOG_AUDIO_DEBUG("Moving from index %d to %d in custom list", currentFileIndex, prevIndex);
        
        if (playFileByIndex(prevIndex)) {
            LOG_AUDIO_INFO("Previous file started successfully (CUSTOM mode)");
            return true;
        } else {
            LOG_AUDIO_WARN("Failed to play previous file (CUSTOM mode), restarting from first...");
            setLastError("Failed to play previous file (CUSTOM), restarting from first");
            
            // Try to restart from the first file in custom list
            return playFileByIndex(0);
        }
    }
}

// Check if playing
bool Audio_Manager::isPlaying() const {
    return player && playerActive && player->isActive();
}

// Check and update playback state (call this before checking isPlaying)
void Audio_Manager::updatePlaybackState() {
    if (!player) return;
    
    // Check if playerActive flag matches actual player state
    bool actuallyActive = player->isActive();
    if (playerActive != actuallyActive) {
        LOG_AUDIO_DEBUG("Updating playback state: playerActive=%s -> %s", 
                        playerActive ? "true" : "false", actuallyActive ? "true" : "false");
        playerActive = actuallyActive;
        
        // If playback ended naturally, clear current file
        if (!actuallyActive && currentFile.length() > 0) {
            LOG_AUDIO_DEBUG("Playback ended naturally, clearing current file");
            currentFile = "";
        }
    }
}

// Check if paused
bool Audio_Manager::isPaused() const {
    return !playerActive && currentFile.length() > 0;
}

// Check if stopped
bool Audio_Manager::isStopped() const {
    return !playerActive && currentFile.length() == 0;
}

// Set volume
void Audio_Manager::setVolume(float volume) {
    currentVolume = constrain(volume, 0.0f, 1.0f);
    if (this->volume) {
        this->volume->setVolume(currentVolume);
    }
    LOG_AUDIO_DEBUG("Volume set to: %.2f", currentVolume);
}

// Get current volume
float Audio_Manager::getVolume() const {
    return currentVolume;
}

// Get current file
String Audio_Manager::getCurrentFile() const {
    return currentFile;
}

// Get first audio file
String Audio_Manager::getFirstAudioFile() const {
    return firstAudioFile;
}

// Check if files are available
bool Audio_Manager::areFilesAvailable() const {
    return filesAvailable;
}

// Get file count
int Audio_Manager::getFileCount() const {
    return totalAudioFiles;
}

// Update function (call in main loop)
void Audio_Manager::update() {
    if (!audioInitialized || !player) {
        return;
    }
    
    // Debug: Print state every 1000ms
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 1000) {
        //Serial.printf("DEBUG: playerActive=%s, player->isActive()=%s\n", 
        //            playerActive ? "true" : "false",
        //            player->isActive() ? "true" : "false");
        lastDebug = millis();
    }
    
    // Handle audio playback
    if (playerActive && player->isActive()) {
        try {
            //Serial.println("DEBUG: Calling player->copy()");  // Add this line
            player->copy();
        } catch (const std::exception& e) {
            LOG_AUDIO_ERROR("Exception during audio copy");
            stopPlayback();
        }
    } else if (playerActive && !player->isActive()) {
        // Playback has naturally ended - update state
        LOG_AUDIO_DEBUG("Playback naturally ended, updating state... (playerActive=%s, currentFile='%s')", 
                        playerActive ? "true" : "false", currentFile.c_str());
        playerActive = false;
        currentFile = "";
    } else {
        // Debug: Why not playing?
        //if (!playerActive) Serial.println("DEBUG: playerActive is false");
        //if (!player->isActive()) Serial.println("DEBUG: player->isActive() is false");
    }
}

// Set I2S pins
void Audio_Manager::setI2SPins(uint8_t bck, uint8_t ws, uint8_t data) {
    i2sBckPin = bck;
    i2sWsPin = ws;
    i2sDataPin = data;
}

// Set buffer settings
void Audio_Manager::setBufferSettings(uint16_t bufferSize, uint8_t bufferCount) {
    i2sBufferSize = bufferSize;
    i2sBufferCount = bufferCount;
}

// Set audio folder
void Audio_Manager::setAudioFolder(const char* folder) {
    audioFolder = folder;
}

// Set file extension
void Audio_Manager::setFileExtension(const char* ext) {
    fileExtension = ext;
}

// Print audio status
void Audio_Manager::printAudioStatus() const {
    LOG_AUDIO_INFO("=== Audio Status ===");
    LOG_AUDIO_INFO("Initialized: %s", audioInitialized ? "Yes" : "No");
    LOG_AUDIO_INFO("File Selection Mode: %s", 
                   (fileSelectionMode == FileSelectionMode::BUILTIN) ? "BUILTIN" : "CUSTOM");
    LOG_AUDIO_INFO("Files Available: %s", filesAvailable ? "Yes" : "No");
    LOG_AUDIO_INFO("Total Files: %d", totalAudioFiles);
    LOG_AUDIO_INFO("Current File: %s", currentFile.length() > 0 ? currentFile.c_str() : "None");
    LOG_AUDIO_INFO("Player Active: %s", playerActive ? "Yes" : "No");
    LOG_AUDIO_INFO("Volume: %.2f", currentVolume);
    LOG_AUDIO_INFO("I2S Pins: BCK=%d, WS=%d, DATA=%d", i2sBckPin, i2sWsPin, i2sDataPin);
    
    // Add custom mode specific information
    if (fileSelectionMode == FileSelectionMode::CUSTOM) {
        LOG_AUDIO_INFO("Custom File List Size: %d", audioFileList.size());
        LOG_AUDIO_INFO("Current File Index: %d", currentFileIndex);
        if (audioFileList.size() > 0) {
            LOG_AUDIO_INFO("Custom File List:");
            for (int i = 0; i < audioFileList.size(); i++) {
                String marker = (i == currentFileIndex) ? " -> " : "    ";
                LOG_AUDIO_INFO("%s%d. %s", marker.c_str(), i, audioFileList[i].c_str());
            }
        }
    }
    
    LOG_AUDIO_INFO("==================");
}

// Print file list
void Audio_Manager::printFileList() const {
    if (!filesListed) {
        LOG_AUDIO_WARN("Files not yet listed");
        return;
    }
    
    LOG_AUDIO_INFO("=== Audio Files in %s ===", audioFolder);
    LOG_AUDIO_INFO("Total files: %d", totalAudioFiles);
    if (firstAudioFile.length() > 0) {
        LOG_AUDIO_INFO("First file: %s", firstAudioFile.c_str());
    }
    LOG_AUDIO_INFO("======================");
}

// Get last error
const char* Audio_Manager::getLastError() const {
    return lastError;
}

// Send silence to clear audio pipeline
void Audio_Manager::sendSilence(int frames) {
    if (!volume) return;
    
    int16_t silence[64];
    memset(silence, 0, sizeof(silence));
    
    for (int i = 0; i < frames / 32; i++) {
        volume->write((uint8_t*)silence, sizeof(silence));
        delay(1);
    }
}

// Clear audio pipeline
void Audio_Manager::clearAudioPipeline() {
    sendSilence(100);
    if (i2s) i2s->flush();
}

// Set last error
void Audio_Manager::setLastError(const char* error) const {
    if (error) {
        strncpy(lastError, error, sizeof(lastError) - 1);
        lastError[sizeof(lastError) - 1] = '\0';
    } else {
        strcpy(lastError, "Unknown error");
    }
}

// Set file selection mode
void Audio_Manager::setFileSelectionMode(FileSelectionMode mode) {
    fileSelectionMode = mode;
    LOG_AUDIO_INFO("File selection mode changed to: %s", 
                   (mode == FileSelectionMode::BUILTIN) ? "BUILTIN" : "CUSTOM");
}

// Build custom file list for CUSTOM mode
bool Audio_Manager::buildCustomFileList() {
    if (fileSelectionMode != FileSelectionMode::CUSTOM) {
        LOG_AUDIO_DEBUG("buildCustomFileList: Not in CUSTOM mode, skipping");
        return true; // Not needed for BUILTIN mode
    }
    
    LOG_AUDIO_DEBUG("Building custom file list...");
    LOG_AUDIO_DEBUG("Audio folder: %s", audioFolder);
    LOG_AUDIO_DEBUG("File extension: %s", fileExtension);
    
    audioFileList.clear();
    currentFileIndex = 0;
    
    // Handle root directory (empty string)
    const char* folderPath = (audioFolder[0] == '\0') ? "/" : audioFolder;
    LOG_AUDIO_DEBUG("Scanning folder: %s", folderPath);
    
    if (!SD_MMC.exists(folderPath)) {
        LOG_AUDIO_ERROR("Audio folder %s does not exist!", folderPath);
        setLastError("Audio folder not found for custom list");
        return false;
    }
    
    File folder = SD_MMC.open(folderPath);
    if (!folder || !folder.isDirectory()) {
        LOG_AUDIO_ERROR("Failed to open folder %s", folderPath);
        setLastError("Failed to open audio folder for custom list");
        return false;
    }
    
    LOG_AUDIO_DEBUG("Folder opened successfully, scanning files...");
    int fileCount = 0;
    File file = folder.openNextFile();
    
    while (file) {
        if (!file.isDirectory()) {
            String filename = String(file.name());
            LOG_AUDIO_DEBUG("Found file: %s (starts with '_': %s, ends with %s: %s)", 
                            filename.c_str(), 
                            filename.startsWith("_") ? "yes" : "no",
                            fileExtension,
                            filename.endsWith(fileExtension) ? "yes" : "no");
            
            // Only include .mp3 files that don't start with "_"
            if (filename.endsWith(fileExtension) && !filename.startsWith("_")) {
                audioFileList.push_back(filename);
                fileCount++;
                LOG_AUDIO_DEBUG("Added to custom list: %s", filename.c_str());
            }
        }
        file.close();
        file = folder.openNextFile();
    }
    
    folder.close();
    
    LOG_AUDIO_DEBUG("Custom file list built with %d files", fileCount);
    LOG_AUDIO_DEBUG("Custom file list contents:");
    for (int i = 0; i < audioFileList.size(); i++) {
        LOG_AUDIO_DEBUG("  %d: %s", i, audioFileList[i].c_str());
    }
    
    return fileCount > 0;
}

// Play file by index in custom list
bool Audio_Manager::playFileByIndex(int index) {
    if (fileSelectionMode != FileSelectionMode::CUSTOM) {
        setLastError("Not in CUSTOM mode");
        return false;
    }
    
    if (index < 0 || index >= audioFileList.size()) {
        setLastError("Invalid file index");
        return false;
    }
    
    String filename = audioFileList[index];
    currentFileIndex = index;
    
    LOG_AUDIO_INFO("Playing custom list file %d: %s", index, filename.c_str());
    
    // Stop current playback
    stopPlayback();
    delay(50);
    
    try {
        // Build full path
        String fullPath = String(audioFolder) + "/" + filename;
        LOG_AUDIO_DEBUG("Full path: %s", fullPath.c_str());
        
        // Use playPath for custom mode
        if (player->playPath(fullPath.c_str())) {
            currentFile = filename;
            playerActive = true;
            LOG_AUDIO_INFO("Started playing custom file: %s", filename.c_str());
            return true;
        } else {
            setLastError("Failed to play custom file");
            return false;
        }
        
    } catch (const std::exception& e) {
        setLastError("Exception during custom file playback");
        return false;
    }
}

// Find file index in custom list
int Audio_Manager::findFileIndex(const String& filename) {
    if (fileSelectionMode != FileSelectionMode::CUSTOM) {
        return -1;
    }
    
    for (int i = 0; i < audioFileList.size(); i++) {
        if (audioFileList[i] == filename) {
            return i;
        }
    }
    return -1; // Not found
}

// Change audio source to a new folder
bool Audio_Manager::changeAudioSource(const char* newFolder) {
    if (!audioInitialized) {
        setLastError("Audio Manager not initialized");
        return false;
    }
    
    LOG_AUDIO_INFO("Changing audio source from '%s' to '%s'", audioFolder, newFolder);
    
    // Check if we're already using this folder
    if (strcmp(audioFolder, newFolder) == 0) {
        LOG_AUDIO_DEBUG("Already using this audio source, no change needed");
        return true;
    }
    
    // Stop current playback
    stopPlayback();
    delay(100); // Give time for cleanup
    
    // Update the audio folder
    audioFolder = newFolder;
    
    // Clear existing file information
    firstAudioFile = "";
    currentFile = "";
    currentFileIndex = 0;
    audioFileList.clear();
    filesListed = false;
    filesAvailable = false;
    totalAudioFiles = 0;
    
    // Delete and recreate the audio source
    if (source) {
        delete source;
        source = nullptr;
        delay(10); // Small delay for cleanup
    }
    
    // Handle root directory (empty string)
    const char* sourcePath = (audioFolder[0] == '\0') ? "/" : audioFolder;
    
    try {
        // Create new AudioSourceSDMMC
        source = new AudioSourceSDMMC(sourcePath, fileExtension);
        if (!source) {
            setLastError("Failed to create new audio source");
            return false;
        }
        LOG_AUDIO_DEBUG("New audio source created for path: %s", sourcePath);
        
        // Update the player's source
        if (player && source) {
            // Stop the player first
            player->stop();
            
            // Use setAudioSource to change the source without recreating the player
            player->setAudioSource(*source);
            
            LOG_AUDIO_INFO("Audio player source updated successfully");
        } else {
            setLastError("Player or source not initialized");
            return false;
        }
        
        // Relist audio files in the new folder
        if (!listAudioFiles()) {
            LOG_AUDIO_ERROR("Failed to list audio files in new folder: %s", getLastError());
            setLastError("Failed to list files in new folder");
            return false;
        }
        
        LOG_AUDIO_INFO("Audio source changed successfully to: %s", audioFolder);
        return true;
        
    } catch (const std::exception& e) {
        setLastError("Exception during audio source change");
        return false;
    }
}
