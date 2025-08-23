#include "Audio_Manager.h"

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
    Serial.println("Initializing Audio Manager...");
    Serial.printf("Audio folder: %s\n", audioFolder);
    Serial.printf("File extension: %s\n", fileExtension);
    
    // Initialize I2S
    if (!initializeI2S()) {
        Serial.printf("Failed to initialize I2S: %s\n", getLastError());
        return false;
    }
    
    // Initialize audio pipeline
    if (!initializeAudioPipeline()) {
        Serial.printf("Failed to initialize audio pipeline: %s\n", getLastError());
        return false;
    }
    
    // List audio files
    if (!listAudioFiles()) {
        Serial.printf("Failed to list audio files: %s\n", getLastError());
        return false;
    }
    
    audioInitialized = true;
    Serial.println("Audio Manager initialized successfully!");
    return true;
}

// Initialize I2S
bool Audio_Manager::initializeI2S() {
    Serial.println("Initializing I2S...");

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

    Serial.printf("I2S initialized: BCK=%d, WS=%d, DATA=%d, Buffer=%d, Count=%d\n",
                  i2sBckPin, i2sWsPin, i2sDataPin,
                  i2sCfg_.buffer_size, i2sCfg_.buffer_count);
    return true;
}

// Initialize audio pipeline
bool Audio_Manager::initializeAudioPipeline() {
    Serial.println("Initializing audio pipeline...");
    
    try {
        // Create audio source
        Serial.printf("Creating audio source for folder: %s\n", 
                      audioFolder[0] == '\0' ? "root directory" : audioFolder);
        
        // Handle root directory (empty string)
        const char* sourcePath = (audioFolder[0] == '\0') ? "/" : audioFolder;
        
        // Create AudioSourceSDMMC that will scan the entire folder for audio files
        // This will automatically find all files with the specified extension
        source = new AudioSourceSDMMC(sourcePath, fileExtension);
        
        Serial.printf("Audio source created for path: %s with extension: %s\n", sourcePath, fileExtension);
        
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
        Serial.print("Setting initial volume... ");
        Serial.println(currentVolume);
        volume->setVolume(currentVolume);
        
        Serial.println("Audio pipeline initialized successfully!");
        return true;
        
    } catch (const std::exception& e) {
        setLastError("Exception during audio pipeline initialization");
        return false;
    }
}

// List audio files in the specified folder
bool Audio_Manager::listAudioFiles() {
    Serial.printf("\n=== Listing audio files in %s ===\n", 
                  audioFolder[0] == '\0' ? "root directory" : audioFolder);
    
    // Handle root directory (empty string)
    const char* folderPath = (audioFolder[0] == '\0') ? "/" : audioFolder;
    
    if (!SD_MMC.exists(folderPath)) {
        Serial.printf("Audio folder %s does not exist!\n", folderPath);
        setLastError("Audio folder not found");
        return false;
    }
    
    File folder = SD_MMC.open(folderPath);
    if (!folder || !folder.isDirectory()) {
        Serial.printf("Failed to open folder %s\n", folderPath);
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
                Serial.printf("%d. %s\n", fileCount, filename.c_str());
                
                // Store first audio file found
                if (firstAudioFile.length() == 0) {
                    firstAudioFile = filename;
                    Serial.printf("First audio file: %s\n", firstAudioFile.c_str());
                }
            }
        }
        file.close();
        file = folder.openNextFile();
    }
    
    folder.close();
    
    filesAvailable = (totalAudioFiles > 0);
    filesListed = true;
    
    Serial.printf("Total audio files found: %d\n", totalAudioFiles);
    if (filesAvailable) {
        Serial.printf("Will play first file: %s\n", firstAudioFile.c_str());
        
        // If in CUSTOM mode, build the custom file list
        if (fileSelectionMode == FileSelectionMode::CUSTOM) {
            if (!buildCustomFileList()) {
                Serial.println("Warning: Failed to build custom file list");
            }
        }
    } else {
        Serial.println("No audio files found!");
    }
    
    return filesAvailable;
}

// Play a specific audio file
bool Audio_Manager::playFile(const String& filename) {
    if (!audioInitialized || !filesAvailable) {
        setLastError("Audio system not ready");
        return false;
    }
    
    Serial.printf("Playing file: %s (mode: %s)\n", filename.c_str(),
                  (fileSelectionMode == FileSelectionMode::BUILTIN) ? "BUILTIN" : "CUSTOM");
    
    // Stop current playback
    stopPlayback();
    delay(50); // Brief pause to let everything reset
    
    if (fileSelectionMode == FileSelectionMode::BUILTIN) {
        // BUILTIN mode - use the folder-based approach
        Serial.printf("Starting playback from folder: %s\n", 
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
            Serial.printf("Started playing: %s (BUILTIN mode)\n", filename.c_str());
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
            Serial.printf("Full path for custom mode: %s\n", fullPath.c_str());
            
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
                    Serial.printf("Updated current file index to: %d\n", currentFileIndex);
                }
                
                Serial.printf("Started playing: %s (CUSTOM mode)\n", filename.c_str());
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
    
    Serial.println("Stopping playback...");
    
    try {
        player->stop();
        playerActive = false;
        currentFile = "";
        
        // Send silence to clear the audio pipeline
        sendSilence(500);
        
        // Flush I2S
        if (i2s) i2s->flush();
        
        Serial.println("Playback stopped");
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
    
    Serial.println("Pausing playback...");
    
    // Store current file info before stopping (for potential resume)
    String pausedFile = currentFile;
    
    // Simply stop the player
    player->stop();
    playerActive = false;
    
    // Keep track of the paused file for potential resume
    if (pausedFile.length() > 0) {
        Serial.printf("Playback paused on file: %s\n", pausedFile.c_str());
    }
    
    Serial.println("Playback paused");
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
        Serial.println("Player is already active, no need to resume");
        return true;
    }
    
    // If playerActive flag is true but player is not active, reset the flag
    if (playerActive && !player->isActive()) {
        Serial.println("Resetting stale playerActive flag...");
        playerActive = false;
        currentFile = "";
    }
    
    // If we don't have a current file, we can't resume - need to restart
    if (currentFile.length() == 0) {
        Serial.println("No current file to resume from, restarting from first file...");
        setLastError("No current file, restarting from first");
        
        // Try to restart from the first file in the current folder
        if (fileSelectionMode == FileSelectionMode::BUILTIN) {
            // BUILTIN mode - restart from folder beginning
            if (filesAvailable && firstAudioFile.length() > 0) {
                Serial.printf("Restarting playback from first file: %s\n", firstAudioFile.c_str());
                return playFile(firstAudioFile);
            } else {
                Serial.println("No first file available for restart (BUILTIN mode)");
                setLastError("No first file available for restart");
                return false;
            }
        } else {
            // CUSTOM mode - restart from first file in custom list
            if (audioFileList.size() > 0) {
                Serial.printf("Restarting playback from first file in custom list: %s\n", audioFileList[0].c_str());
                return playFileByIndex(0);
            } else {
                Serial.println("No files in custom list for restart");
                setLastError("No files in custom list for restart");
                return false;
            }
        }
    }
    
    Serial.println("Resuming playback...");
    
    // Try to resume using AudioPlayer's simple play() method
    player->play();
    playerActive = true;
    
    // Wait a moment and check if playback actually started
    delay(100);
    
    // Check if the player is actually active and working
    bool isActive = player->isActive();
    Serial.printf("After resume attempt: player->isActive() = %s\n", isActive ? "true" : "false");
    
    if (isActive) {
        Serial.println("Playback resumed successfully");
        return true;
    } else {
        Serial.println("Resume failed - player not active, attempting to restart from first file...");
        setLastError("Resume failed, restarting from first file");
        
        // Try to restart from the first file in the current folder
        if (fileSelectionMode == FileSelectionMode::BUILTIN) {
            // BUILTIN mode - restart from folder beginning
            if (filesAvailable && firstAudioFile.length() > 0) {
                Serial.printf("Restarting playback from first file: %s\n", firstAudioFile.c_str());
                return playFile(firstAudioFile);
            } else {
                Serial.println("No first file available for restart (BUILTIN mode)");
                setLastError("No first file available for restart");
                return false;
            }
        } else {
            // CUSTOM mode - restart from first file in custom list
            if (audioFileList.size() > 0) {
                Serial.printf("Restarting playback from first file in custom list: %s\n", audioFileList[0].c_str());
                return playFileByIndex(0);
            } else {
                Serial.println("No files in custom list for restart");
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
    Serial.println("Force restarting from first file...");
    
    if (fileSelectionMode == FileSelectionMode::BUILTIN) {
        // BUILTIN mode - restart from folder beginning
        if (filesAvailable && firstAudioFile.length() > 0) {
            Serial.printf("Restarting playback from first file: %s\n", firstAudioFile.c_str());
            return playFile(firstAudioFile);
        } else {
            Serial.println("No first file available for restart (BUILTIN mode)");
            setLastError("No first file available for restart");
            return false;
        }
    } else {
        // CUSTOM mode - restart from first file in custom list
        if (audioFileList.size() > 0) {
            Serial.printf("Restarting playback from first file in custom list: %s\n", audioFileList[0].c_str());
            return playFileByIndex(0);
        } else {
            Serial.println("No files in custom list for restart");
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
    
    Serial.printf("Moving to next file (mode: %s)...\n", 
                  (fileSelectionMode == FileSelectionMode::BUILTIN) ? "BUILTIN" : "CUSTOM");
    
    if (fileSelectionMode == FileSelectionMode::BUILTIN) {
        // Use AudioPlayer's built-in next() method
        if (player->next(1)) {
            playerActive = true;
            Serial.println("Next file started successfully (BUILTIN mode)");
            return true;
        } else {
            Serial.println("Failed to move to next file (BUILTIN mode), restarting from first file...");
            setLastError("Failed to move to next file (BUILTIN), restarting from first");
            
            // Try to restart from the first file
            if (filesAvailable && firstAudioFile.length() > 0) {
                Serial.printf("Restarting playback from first file: %s\n", firstAudioFile.c_str());
                return playFile(firstAudioFile);
            } else {
                Serial.println("No first file available for restart");
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
        Serial.printf("Moving from index %d to %d in custom list\n", currentFileIndex, nextIndex);
        
        if (playFileByIndex(nextIndex)) {
            Serial.println("Next file started successfully (CUSTOM mode)");
            return true;
        } else {
            Serial.println("Failed to play next file (CUSTOM mode), restarting from first...");
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
    
    Serial.printf("Moving to previous file (mode: %s)...\n", 
                  (fileSelectionMode == FileSelectionMode::BUILTIN) ? "BUILTIN" : "CUSTOM");
    
    if (fileSelectionMode == FileSelectionMode::BUILTIN) {
        // Use AudioPlayer's built-in previous() method
        if (player->previous(1)) {
            playerActive = true;
            Serial.println("Previous file started successfully (BUILTIN mode)");
            return true;
        } else {
            Serial.println("Failed to move to previous file (BUILTIN mode), restarting from first file...");
            setLastError("Failed to move to previous file (BUILTIN), restarting from first");
            
            // Try to restart from the first file
            if (filesAvailable && firstAudioFile.length() > 0) {
                Serial.printf("Restarting playback from first file: %s\n", firstAudioFile.c_str());
                return playFile(firstAudioFile);
            } else {
                Serial.println("No first file available for restart");
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
        Serial.printf("Moving from index %d to %d in custom list\n", currentFileIndex, prevIndex);
        
        if (playFileByIndex(prevIndex)) {
            Serial.println("Previous file started successfully (CUSTOM mode)");
            return true;
        } else {
            Serial.println("Failed to play previous file (CUSTOM mode), restarting from first...");
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
        Serial.printf("Updating playback state: playerActive=%s -> %s\n", 
                      playerActive ? "true" : "false", actuallyActive ? "true" : "false");
        playerActive = actuallyActive;
        
        // If playback ended naturally, clear current file
        if (!actuallyActive && currentFile.length() > 0) {
            Serial.println("Playback ended naturally, clearing current file");
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
    Serial.printf("Volume set to: %.2f\n", currentVolume);
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
            Serial.println("Exception during audio copy");
            stopPlayback();
        }
    } else if (playerActive && !player->isActive()) {
        // Playback has naturally ended - update state
        Serial.printf("Playback naturally ended, updating state... (playerActive=%s, currentFile='%s')\n", 
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
    Serial.println("\n=== Audio Status ===");
    Serial.printf("Initialized: %s\n", audioInitialized ? "Yes" : "No");
    Serial.printf("File Selection Mode: %s\n", 
                  (fileSelectionMode == FileSelectionMode::BUILTIN) ? "BUILTIN" : "CUSTOM");
    Serial.printf("Files Available: %s\n", filesAvailable ? "Yes" : "No");
    Serial.printf("Total Files: %d\n", totalAudioFiles);
    Serial.printf("Current File: %s\n", currentFile.length() > 0 ? currentFile.c_str() : "None");
    Serial.printf("Player Active: %s\n", playerActive ? "Yes" : "No");
    Serial.printf("Volume: %.2f\n", currentVolume);
    Serial.printf("I2S Pins: BCK=%d, WS=%d, DATA=%d\n", i2sBckPin, i2sWsPin, i2sDataPin);
    
    // Add custom mode specific information
    if (fileSelectionMode == FileSelectionMode::CUSTOM) {
        Serial.printf("Custom File List Size: %d\n", audioFileList.size());
        Serial.printf("Current File Index: %d\n", currentFileIndex);
        if (audioFileList.size() > 0) {
            Serial.println("Custom File List:");
            for (int i = 0; i < audioFileList.size(); i++) {
                String marker = (i == currentFileIndex) ? " -> " : "    ";
                Serial.printf("%s%d. %s\n", marker.c_str(), i, audioFileList[i].c_str());
            }
        }
    }
    
    Serial.println("==================\n");
}

// Print file list
void Audio_Manager::printFileList() const {
    if (!filesListed) {
        Serial.println("Files not yet listed");
        return;
    }
    
    Serial.printf("\n=== Audio Files in %s ===\n", audioFolder);
    Serial.printf("Total files: %d\n", totalAudioFiles);
    if (firstAudioFile.length() > 0) {
        Serial.printf("First file: %s\n", firstAudioFile.c_str());
    }
    Serial.println("======================\n");
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
    Serial.printf("File selection mode changed to: %s\n", 
                  (mode == FileSelectionMode::BUILTIN) ? "BUILTIN" : "CUSTOM");
}

// Build custom file list for CUSTOM mode
bool Audio_Manager::buildCustomFileList() {
    if (fileSelectionMode != FileSelectionMode::CUSTOM) {
        Serial.println("buildCustomFileList: Not in CUSTOM mode, skipping");
        return true; // Not needed for BUILTIN mode
    }
    
    Serial.println("Building custom file list...");
    Serial.printf("Audio folder: %s\n", audioFolder);
    Serial.printf("File extension: %s\n", fileExtension);
    
    audioFileList.clear();
    currentFileIndex = 0;
    
    // Handle root directory (empty string)
    const char* folderPath = (audioFolder[0] == '\0') ? "/" : audioFolder;
    Serial.printf("Scanning folder: %s\n", folderPath);
    
    if (!SD_MMC.exists(folderPath)) {
        Serial.printf("Audio folder %s does not exist!\n", folderPath);
        setLastError("Audio folder not found for custom list");
        return false;
    }
    
    File folder = SD_MMC.open(folderPath);
    if (!folder || !folder.isDirectory()) {
        Serial.printf("Failed to open folder %s\n", folderPath);
        setLastError("Failed to open audio folder for custom list");
        return false;
    }
    
    Serial.println("Folder opened successfully, scanning files...");
    int fileCount = 0;
    File file = folder.openNextFile();
    
    while (file) {
        if (!file.isDirectory()) {
            String filename = String(file.name());
            Serial.printf("Found file: %s (starts with '_': %s, ends with %s: %s)\n", 
                        filename.c_str(), 
                        filename.startsWith("_") ? "yes" : "no",
                        fileExtension,
                        filename.endsWith(fileExtension) ? "yes" : "no");
            
            // Only include .mp3 files that don't start with "_"
            if (filename.endsWith(fileExtension) && !filename.startsWith("_")) {
                audioFileList.push_back(filename);
                fileCount++;
                Serial.printf("Added to custom list: %s\n", filename.c_str());
            }
        }
        file.close();
        file = folder.openNextFile();
    }
    
    folder.close();
    
    Serial.printf("Custom file list built with %d files\n", fileCount);
    Serial.println("Custom file list contents:");
    for (int i = 0; i < audioFileList.size(); i++) {
        Serial.printf("  %d: %s\n", i, audioFileList[i].c_str());
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
    
    Serial.printf("Playing custom list file %d: %s\n", index, filename.c_str());
    
    // Stop current playback
    stopPlayback();
    delay(50);
    
    try {
        // Build full path
        String fullPath = String(audioFolder) + "/" + filename;
        Serial.printf("Full path: %s\n", fullPath.c_str());
        
        // Use playPath for custom mode
        if (player->playPath(fullPath.c_str())) {
            currentFile = filename;
            playerActive = true;
            Serial.printf("Started playing custom file: %s\n", filename.c_str());
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
    
    Serial.printf("Changing audio source from '%s' to '%s'\n", audioFolder, newFolder);
    
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
    }
    
    // Handle root directory (empty string)
    const char* sourcePath = (audioFolder[0] == '\0') ? "/" : audioFolder;
    
    try {
        // Create new AudioSourceSDMMC
        source = new AudioSourceSDMMC(sourcePath, fileExtension);
        Serial.printf("New audio source created for path: %s\n", sourcePath);
        
        // Update the player's source
        if (player) {
            // Stop the player first
            player->stop();
            
            // Create a new player with the new source
            delete player;
            player = new AudioPlayer(*source, *volume, *decoder);
            player->setBufferSize(i2sCfg_.buffer_size);
            
            Serial.println("Audio player updated with new source");
        }
        
        // Relist audio files in the new folder
        if (!listAudioFiles()) {
            Serial.printf("Failed to list audio files in new folder: %s\n", getLastError());
            setLastError("Failed to list files in new folder");
            return false;
        }
        
        Serial.printf("Audio source changed successfully to: %s\n", audioFolder);
        return true;
        
    } catch (const std::exception& e) {
        setLastError("Exception during audio source change");
        return false;
    }
}
