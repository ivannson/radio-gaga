/*
 * Audio_Manager Usage Examples
 * 
 * This file demonstrates how to use the Audio_Manager class
 * for audio playback from SD card using arduino-audio-tools.
 */

#include "Audio_Manager.h"

// Create audio manager instance
Audio_Manager audioManager("/test_audio", "mp3");

void setup() {
    Serial.begin(115200);
    Serial.println("Audio Manager Test");
    
    // Initialize audio manager
    if (!audioManager.begin()) {
        Serial.printf("Failed to initialize audio manager: %s\n", audioManager.getLastError());
        while(1) delay(1000);
    }
    
    // Print audio status
    audioManager.printAudioStatus();
    audioManager.printFileList();
    
    // Try to play the first audio file found
    if (audioManager.areFilesAvailable()) {
        String firstFile = audioManager.getFirstAudioFile();
        Serial.printf("Attempting to play first file: %s\n", firstFile.c_str());
        
        if (audioManager.playFile(firstFile)) {
            Serial.println("Playback started successfully!");
        } else {
            Serial.printf("Failed to start playback: %s\n", audioManager.getLastError());
        }
    } else {
        Serial.println("No audio files available for playback");
    }
    
    Serial.println("Audio system ready!");
}

void loop() {
    // Update audio manager (required for playback)
    audioManager.update();
    
    // Print status every 5 seconds
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 5000) {
        audioManager.printAudioStatus();
        lastStatusPrint = millis();
    }
    
    // Small delay for responsiveness
    delay(10);
}

/*
 * Advanced Usage Examples:
 * 
 * // Custom I2S pin configuration
 * audioManager.setI2SPins(26, 25, 32); // BCK, WS, DATA
 * 
 * // Custom buffer settings
 * audioManager.setBufferSettings(2048, 4); // buffer size, count
 * 
 * // Change audio folder
 * audioManager.setAudioFolder("/music");
 * 
 * // Change file extension
 * audioManager.setFileExtension("wav");
 * 
 * // Volume control
 * audioManager.setVolume(0.5f); // 50%
 * 
 * // Playback control
 * if (audioManager.isPlaying()) {
 *     audioManager.stopPlayback();
 * } else if (audioManager.isPaused()) {
 *     audioManager.resumePlayback();
 * }
 * 
 * // File information
 * int fileCount = audioManager.getFileCount();
 * String currentFile = audioManager.getCurrentFile();
 * 
 * // Error handling
 * if (!audioManager.isInitialized()) {
 *     Serial.printf("Audio error: %s\n", audioManager.getLastError());
 * }
 */
