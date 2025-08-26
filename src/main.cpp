#include <Arduino.h>
#include <FastLED.h>
#include <AudioTools.h>
#include "AudioTools/Disk/AudioSourceSDMMC.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "SD_MMC.h"
#include <Adafruit_TLV320DAC3100.h>
#include "Adafruit_TLV320DAC3100_typedefs.h"
#include <Wire.h>
#include "driver/gpio.h" // For pull-ups
#include <SPI.h>
#include "SD_Manager.h"
#include "DAC_Manager.h"
#include "Button_Manager.h"
#include "Rotary_Manager.h"
#include "Settings_Manager.h"
#include "Audio_Manager.h"
#include "RFID_Manager.h"
#include "Battery_Manager.h"
#include "SetupMode.h"
#include "SdScanner.h"
#include "MappingStore.h"

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

#define WLED_PIN 13
#define BRIGHTNESS 50
#define NUM_LEDS 1



// SD MMC Pins (1-bit mode)
#define SD_MMC_CMD 15  // Command line
#define SD_MMC_CLK 14  // Clock line
#define SD_MMC_D0  2   // Data line 0 (only one needed for 1-bit mode)

// SPI Pins for RFID MFRC522
#define SPI_SCLK 18    // SPI Clock line
#define SPI_MISO 23    // SPI MISO line
#define SPI_MOSI 19    // SPI MOSI line
#define SPI_SS   5     // SPI SS line (RFID SDA)

// TLV320DAC3100 Configuration
#define TLV_RESET 4    // Reset pin
#define I2C_SDA 22     // I2C SDA pin
#define I2C_SCL 21     // I2C SCL pin

// Button Configuration
#define ADC_BUTTONS_PIN 39  // ADC pin for 4 buttons

// Rotary Encoder Configuration
#define ROTARY_CLK_PIN 27   // Encoder CLK pin
#define ROTARY_DT_PIN 34    // Encoder DT pin

// Manager instances
SD_Manager sdManager(true, "/sdcard");  // one_bit_mode, mount_point
DAC_Manager dacManager(TLV_RESET, I2C_SDA, I2C_SCL, 0x18);  // reset_pin, sda_pin, scl_pin, i2c_address
Button_Manager buttonManager(ADC_BUTTONS_PIN, 1.74f, 1.35f, 0.80f, 0.39f);  // adc_pin, encoder, previous, play_pause, next
Rotary_Manager rotaryManager(ROTARY_CLK_PIN, ROTARY_DT_PIN, -1);  // clk_pin, dt_pin, button_pin (-1 for ADC button)
Settings_Manager settingsManager("/settings.json");  // settings file path
RFID_Manager rfidManager(SPI_SCLK, SPI_MISO, SPI_MOSI, SPI_SS);  // sclk, miso, mosi, ss
Battery_Manager batteryManager; // Battery manager instance

// Setup mode components
SdScanner sdScanner;
MappingStore mappingStore;
SetupMode setupMode;

// ============================================================================
// AUDIO MANAGER CONFIGURATION
// ============================================================================
// File Selection Modes:
// - FileSelectionMode::BUILTIN: Uses AudioPlayer's built-in next/previous methods
// - FileSelectionMode::CUSTOM: Uses custom file list with playPath() for specific file control
// 
// To switch modes, change the third parameter:
// - BUILTIN: audioManager("/test_music", "mp3", FileSelectionMode::BUILTIN)
// - CUSTOM:  audioManager("/test_music", "mp3", FileSelectionMode::CUSTOM)
// 
// CUSTOM mode filters out files starting with "_" and provides more control over file selection
// ============================================================================

Audio_Manager audioManager("/test_music", "mp3", FileSelectionMode::BUILTIN);  // audio folder, file extension, file selection mode

// ============================================================================
// MANAGER INSTANCES
// ============================================================================

// ============================================================================
// GLOBAL HEADPHONE/SPEAKER ROUTING
// ============================================================================

// Headphone routing is now handled by DAC_Manager.updateHeadphoneRouting()
// The old global variables and constants have been removed

// --- Simple GPIO-based headphone detection using GPIO33
static const uint8_t HP_GPIO_PIN = 33;  // GPIO33 connected to DAC MIC pin
static const uint16_t HP_READ_INTERVAL = 100;  // Read every 100ms
static const uint8_t HP_CONSECUTIVE_READS = 3;  // Need 3 consecutive readings to confirm change

static bool g_currentHPState = false;     // Current headphone state (true=headphones, false=speaker)
static bool g_targetHPState = false;      // Target state we're moving toward
static uint8_t g_consecutiveCount = 0;    // Count of consecutive readings in target state
static uint32_t g_lastHPRead = 0;         // Last time we read the GPIO
static bool g_lastGPIOReading = false;    // Previous GPIO reading to detect changes

static void applyRoute(bool hp) {
  Serial.printf("[HP-DET] applyRoute called with hp=%s\n", hp ? "true" : "false");
  
  // Disable speaker when headphones are present, enable when they're not
  bool speakerShouldBeEnabled = !hp;
  Serial.printf("[HP-DET] Speaker should be %s\n", speakerShouldBeEnabled ? "ENABLED" : "DISABLED");
  
  
  
  // Also set speaker volume to 0 when disabled to ensure it's really off
  if (!speakerShouldBeEnabled) {
    dacManager.setSpeakerVolume(0);
    Serial.println("[HP-DET] Speaker volume set to 0 for complete muting");
  } else {
    audioManager.pausePlayback();  // Pause audio playback when switching routes
    delay(250);  // Small delay to ensure audio stops before changing route
    // Restore speaker volume to default when enabling speaker output
    dacManager.setSpeakerVolume(6); // Restore normal volume
    Serial.println("[HP-DET] Speaker volume restored to 6");
  }

  dacManager.enableSpeaker(speakerShouldBeEnabled);
  
  Serial.printf("[HP-DET] ROUTE -> %s (Speaker: %s)\n", 
                hp ? "HEADPHONES" : "SPEAKER",
                speakerShouldBeEnabled ? "ON" : "OFF");
}

static void updateOutputRoute(bool force = false) {
  const uint32_t now = millis();
  
  // Read GPIO at regular intervals
  if (now - g_lastHPRead >= HP_READ_INTERVAL) {
    g_lastHPRead = now;
    
    // Read GPIO33 (0=headphones in, 1=headphones out)
    bool gpioReading = !digitalRead(HP_GPIO_PIN); // Invert: 0 becomes true (headphones), 1 becomes false (speaker)
    
    // Only start counting if there's a change from previous reading
    if (gpioReading != g_lastGPIOReading) {
      // Reset count and update target state
      g_consecutiveCount = 1;
      g_targetHPState = gpioReading;
      Serial.printf("[HP-DET] GPIO33: %d -> %s (Starting count: %d/3)\n", 
                    digitalRead(HP_GPIO_PIN), 
                    gpioReading ? "HEADPHONES" : "SPEAKER",
                    g_consecutiveCount);
    } else if (gpioReading == g_targetHPState && g_consecutiveCount < HP_CONSECUTIVE_READS) {
      // Continue counting if same state and not yet reached 3
      g_consecutiveCount++;
      Serial.printf("[HP-DET] Consecutive count: %d/3 for %s\n", 
                    g_consecutiveCount,
                    g_targetHPState ? "HEADPHONES" : "SPEAKER");
    }
    
    // Update previous reading
    g_lastGPIOReading = gpioReading;
    
    // If we have enough consecutive readings, apply the change
    if (g_consecutiveCount >= HP_CONSECUTIVE_READS && g_targetHPState != g_currentHPState) {
      g_currentHPState = g_targetHPState;
      applyRoute(g_currentHPState);
      Serial.printf("[HP-DET] State confirmed: %s after %d consecutive readings\n", 
                    g_currentHPState ? "HEADPHONES" : "SPEAKER", HP_CONSECUTIVE_READS);
    }
  }
  
  // Force initial route on first call
  if (force) {
    // Read current GPIO state and set initial route
    bool initialHP = !digitalRead(HP_GPIO_PIN);
    g_currentHPState = initialHP;
    g_targetHPState = initialHP;
    g_lastGPIOReading = initialHP;
    g_consecutiveCount = HP_CONSECUTIVE_READS; // Start with confirmed state
    
    applyRoute(g_currentHPState);
    Serial.printf("[HP-DET] Initial route: %s (GPIO33: %d)\n", 
                  g_currentHPState ? "HEADPHONES" : "SPEAKER", digitalRead(HP_GPIO_PIN));
  }
}

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// WLED array
CRGB leds[NUM_LEDS];

// SD card status (legacy - now handled by SD_Manager)
bool sdCardMounted = false;

// DAC status
bool dacInitialized = false;

// ============================================================================
// LED FUNCTIONS
// ============================================================================

// Function to flash red LED 3 times for unknown RFID cards
void flashRedLED(int times = 3, int flashDuration = 200, int pauseDuration = 150) {
    for (int i = 0; i < times; i++) {
        // Flash red
        leds[0] = CRGB::Red;
        FastLED.show();
        delay(flashDuration);
        
        // Turn off
        leds[0] = CRGB::Black;
        FastLED.show();
        
        // Pause between flashes (except after last flash)
        if (i < times - 1) {
            delay(pauseDuration);
        }
    }
}

// ============================================================================
// BUTTON HANDLING FUNCTIONS
// ============================================================================

void handleButtonPress(ButtonType buttonType) {
  Serial.printf("handleButtonPress called with button: %d\n", buttonType);
  Serial.printf("Audio Manager initialized: %s\n", audioManager.isInitialized() ? "Yes" : "No");

  // Only require a tag for transport controls, never for the encoder button
  auto requiresTag = [](ButtonType b) {
    return b == BUTTON_PREVIOUS || b == BUTTON_NEXT || b == BUTTON_PLAY_PAUSE;
  };

  const bool tagPresent = rfidManager.isTagPresent();

  if (requiresTag(buttonType) && !tagPresent) {
    Serial.println("No RFID tag present - transport buttons disabled");
    return;
  }

  if (tagPresent) {
    Serial.printf("RFID tag present: %s - buttons enabled\n",
                  rfidManager.getLastDetectedUIDString().c_str());
  }

  switch (buttonType) {
    case BUTTON_PREVIOUS:
      Serial.println("Previous button pressed - going to previous track");
      if (audioManager.isInitialized()) {
        if (!audioManager.playPreviousFile()) {
          Serial.printf("Failed to play previous track: %s\n", audioManager.getLastError());
        }
      } else {
        Serial.println("Audio Manager not initialized, cannot play previous track");
      }
      break;

    case BUTTON_PLAY_PAUSE:
      Serial.println("Play/Pause button pressed - toggling playback");
      if (audioManager.isInitialized()) {
        audioManager.updatePlaybackState();
        if (audioManager.isPlaying()) {
          if (!audioManager.pausePlayback()) {
            Serial.printf("Failed to pause: %s\n", audioManager.getLastError());
          }
        } else {
          if (!audioManager.resumePlayback()) {
            Serial.printf("Failed to resume: %s\n", audioManager.getLastError());
          }
        }
      } else {
        Serial.println("Audio Manager not initialized, cannot toggle playback");
      }
      break;

    case BUTTON_NEXT:
      Serial.println("Next button pressed - going to next track");
      if (audioManager.isInitialized()) {
        if (!audioManager.playNextFile()) {
          Serial.printf("Failed to play next track: %s\n", audioManager.getLastError());
        }
      } else {
        Serial.println("Audio Manager not initialized, cannot play next track");
      }
      break;

    case BUTTON_ENCODER:
      Serial.println("Encoder button pressed - (always active)");
      // Put your always-available action here:
      // e.g., toggle setup mode, cycle volume mode, etc.
      setupMode.enter();  // if this is what you want on encoder press/long-press
      break;

    default:
      Serial.printf("Unknown button type: %d\n", buttonType);
      break;
  }
}
// ============================================================================
// DEBUG FUNCTIONS
// ============================================================================

void listAllSDContents(const char* path, int depth = 0) {
    File root = SD_MMC.open(path);
    if (!root) {
        Serial.printf("Failed to open: %s\n", path);
        return;
    }
    
    if (!root.isDirectory()) {
        Serial.printf("Not a directory: %s\n", path);
        root.close();
        return;
    }
    
    // Print current directory
    String indent = "";
    for (int i = 0; i < depth; i++) {
        indent += "  ";
    }
    Serial.printf("%s📁 %s/\n", indent.c_str(), path);
    
    File file = root.openNextFile();
    while (file) {
        String filename = String(file.name());
        String fullPath = String(path) + "/" + filename;
        
        if (file.isDirectory()) {
            // Recursively list subdirectories
            listAllSDContents(fullPath.c_str(), depth + 1);
        } else {
            // Print file with size
            String fileIndent = "";
            for (int i = 0; i < depth + 1; i++) {
                fileIndent += "  ";
            }
            
            // Get file size
            size_t fileSize = file.size();
            String sizeStr = "";
            if (fileSize < 1024) {
                sizeStr = String(fileSize) + " B";
            } else if (fileSize < 1024 * 1024) {
                sizeStr = String(fileSize / 1024.0, 1) + " KB";
            } else {
                sizeStr = String(fileSize / (1024.0 * 1024.0), 1) + " MB";
            }
            
            Serial.printf("%s📄 %s (%s)\n", fileIndent.c_str(), filename.c_str(), sizeStr.c_str());
        }
        
        file.close();
        file = root.openNextFile();
    }
    
    root.close();
}

// ============================================================================
// MAIN LOOP FUNCTION
// ============================================================================

void loop() {
    // Update button states
    buttonManager.update();
    
    // Update RFID detection (needed for both normal mode and setup mode)
    static uint32_t lastRFID = 0;
    if (millis() - lastRFID >= 100) { // 10 Hz max
        rfidManager.update();
        lastRFID = millis();
    }
    
    // Always tick setup mode (safe: early-returns unless active)
    setupMode.loop();
    
    // If setup mode is active, skip all normal player logic
    if (setupMode.isSetupActive()) {
        return; // Only setup mode runs until it exits
    }
    
    // Enter setup mode on encoder long press release
    if (buttonManager.getButtonState() == BUTTON_RELEASED_LONG &&
        buttonManager.getLastButton() == BUTTON_ENCODER) {
        Serial.println("Encoder long press detected - entering Setup Mode");
        setupMode.enter(); // Handles disabling RFID control etc.
        return; // Skip normal player logic this cycle
    }

      // Handle audio playback first
    if (audioManager.isInitialized()) {
        audioManager.update();
    }
    // check whether to send audio to speaker or headphones
    static uint32_t lastPoll = 0;

    // Update headphone detection every 250ms 
    static uint32_t lastHeadphoneCheck = 0;
    if (millis() - lastHeadphoneCheck >= 250) {
        updateOutputRoute(false);
        lastHeadphoneCheck = millis();
    }

    // Update button states (throttled to prevent audio interference)
    static uint32_t lastBtn = 0;
    static ButtonType lastButtonState = BUTTON_NONE;
    static bool buttonProcessed = false;
    
    if (millis() - lastBtn >= 2) { // 500 Hz max
        
        // Check for button presses and handle them with debouncing
        ButtonType currentButton = buttonManager.getCurrentButton();
        
        if (currentButton != BUTTON_NONE) {
            // Button is pressed
            if (lastButtonState != currentButton) {
                // New button pressed - reset debounce
                lastButtonState = currentButton;
                buttonProcessed = false;
                Serial.printf("New button pressed: %d\n", currentButton);
            }
            
            // Only process if button was just pressed (not held)
            if (!buttonProcessed && buttonManager.isButtonPressed(currentButton)) {
                Serial.printf("Processing button press: %d\n", currentButton);
                handleButtonPress(currentButton);
                buttonProcessed = true;
            }
        } else {
            // No button pressed - reset state
            if (lastButtonState != BUTTON_NONE) {
                lastButtonState = BUTTON_NONE;
                buttonProcessed = false;
                Serial.println("Button released");
            }
        }
        
        lastBtn = millis();
    }
    
    // Update rotary encoder (throttled to prevent audio interference)
    static uint32_t lastRotary = 0;
    if (millis() - lastRotary >= 2) { // 500 Hz max
        rotaryManager.update();
        lastRotary = millis();
    }
    
    // Update battery monitoring (throttled to prevent interference)
    if (batteryManager.isInitialized()) {
        batteryManager.update(); // This handles the 5-second timing internally
    }
    
    // Debug: Print status occasionally
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 5000) { // Every 5 seconds
        //Serial.printf("Encoder status: value=%d, volume=%.2f\n", 
        //            rotaryManager.getEncoderValue(), rotaryManager.getVolume());
        
        // Print audio status
        if (audioManager.isInitialized()) {
            Serial.printf("Audio: %s, File: %s\n", 
                        audioManager.isPlaying() ? "Playing" : "Stopped",
                        audioManager.getCurrentFile().c_str());
        }
        
        // RFID status and audio control status removed to clean up serial output
        
        lastDebug = millis();
    }
    
    // Keep WLED green to show system is working
    if (sdManager.isMounted() && dacInitialized && audioManager.isInitialized()) {
        if (rfidManager.isTagPresent()) {
            leds[0] = CRGB::Blue;  // Blue when RFID tag is present
        } else {
            leds[0] = CRGB::Green; // Green when all systems are ready but no tag
        }
        
        // Add battery status indication (blink pattern)
        static uint32_t lastBatteryBlink = 0;
        static bool batteryBlinkState = false;
        
        if (batteryManager.isInitialized()) {
            unsigned long currentMillis = millis();
            if (currentMillis - lastBatteryBlink >= 2000) { // Every 2 seconds
                lastBatteryBlink = currentMillis;
                batteryBlinkState = !batteryBlinkState;
                
                // Show battery level with blink pattern
                if (batteryBlinkState) {
                    float batteryLevel = batteryManager.getBatteryPercentage();
                    if (batteryLevel > 50.0f) {
                        // Battery good - keep current color
                    } else if (batteryLevel > 20.0f) {
                        // Battery low - blink yellow
                        leds[0] = CRGB::Yellow;
                    } else {
                        // Battery critical - blink red
                        leds[0] = CRGB::Red;
                    }
                }
            }
        }
    } else {
        leds[0] = CRGB::Red;    // Red if any system failed
    }
    FastLED.show();
    
    // Small delay for button responsiveness
    delay(10);
}

// ============================================================================
// MAIN SETUP FUNCTION
// ============================================================================

void setup() {
    Serial.begin(115200);
    AudioLogger::instance().begin(Serial, AudioLogger::Warning);
    
    // Reset system
    delay(100);

    // Initialize WLED
    FastLED.addLeds<WS2812B, WLED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
    
    // Start with LED off
    leds[0] = CRGB::Black;
    FastLED.show();

    // Enable internal pull-ups for 4-bit mode pins (from your working script)
    gpio_pullup_en(GPIO_NUM_2);   // DAT0
    //gpio_pullup_en(GPIO_NUM_4);   // DAT1
    //gpio_pullup_en(GPIO_NUM_12);  // DAT2 - Now used for headphone detection
    //gpio_pullup_en(GPIO_NUM_13);  // DAT3
    gpio_pullup_en(GPIO_NUM_15);  // CMD
    gpio_pullup_dis(GPIO_NUM_14);  // CLK
    
    // Configure GPIO33 as input for headphone detection
    pinMode(33, INPUT_PULLUP);
    Serial.println("GPIO33 configured as input with pull-up for headphone detection");

    // Initialize SD Manager first (needed for audio)
    Serial.println("Initializing SD Manager...");
    if (!sdManager.begin()) {
        Serial.println("Failed to initialize SD Manager!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    sdCardMounted = true;

    // DEBUG: List all files and directories on SD card
    Serial.println("\n=== DEBUG: SD CARD CONTENTS ===");
    listAllSDContents("/");
    Serial.println("=== END DEBUG: SD CARD CONTENTS ===\n");

    // Initialize DAC Manager
    Serial.println("Initializing DAC Manager...");
    if (!dacManager.begin()) {
        Serial.println("Failed to initialize DAC Manager!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    dacInitialized = true;
    
    // Configure DAC with proper volumes
    Serial.println("Configuring DAC...");
    if (!dacManager.configure(true, true, 6, 6)) {  // headphone_detection, speaker_output, hp_vol, spk_vol
        Serial.println("Failed to configure DAC!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    
    // Explicitly disable speaker at startup to ensure known state
    Serial.println("Explicitly disabling speaker at startup...");
    dacManager.enableSpeaker(false);
    dacManager.setSpeakerVolume(0);
    Serial.println("Speaker disabled and muted at startup");
    
    // Wait for DAC and headphone detection to stabilize
    Serial.println("Waiting for DAC and headphone detection to stabilize...");
    delay(500);  // Give headphone detection circuit time to settle
    
    // Default to speaker OFF until we read a real status
    dacManager.enableSpeaker(false);

    // Give the jack-detect block a moment to settle
    delay(150);
    
    // First routing decision using simple GPIO-based headphone detection system
    updateOutputRoute(true);
    
    // Initialize RFID Manager
    if (!rfidManager.begin(true)) {  // Enable self-test
        Serial.println("Failed to initialize RFID Manager");
        return;
    }
    Serial.println("RFID MFRC522 initialized successfully!");

    // Initialize Setup Mode components
    if (!sdScanner.begin(SD_MMC)) {
        Serial.println("Failed to initialize SD Scanner");
        return;
    }
    
    if (!mappingStore.begin(SD_MMC, "/lookup.ndjson")) {
        Serial.println("Failed to initialize Mapping Store");
        return;
    }
    
    if (!setupMode.begin(&mappingStore, &sdScanner, &rfidManager, &buttonManager, "/")) {
        Serial.println("Failed to initialize Setup Mode");
        return;
    }
    Serial.println("Setup Mode components initialized successfully!");
    Serial.println("Scan an RFID card to see the UID!");
    
    // Set up RFID audio control callback
    rfidManager.setAudioControlCallback([](const char* uid, bool tagPresent, bool isNewTag, bool isSameTag) {
        // Suppress audio control during setup mode
        if (setupMode.isSetupActive()) {
            Serial.println("[RFID-AUDIO] Setup mode active - audio control suppressed");
            return;
        }
        
        if (tagPresent) {
            if (isNewTag) {
                Serial.printf("[RFID-AUDIO] New tag detected: %s - Looking up music folder\n", uid);
                
                // Look up the music folder path for this UID
                String musicPath;
                if (mappingStore.getPathFor(uid, musicPath)) {
                    Serial.printf("[RFID-AUDIO] Found mapping: %s -> %s\n", uid, musicPath.c_str());
                    
                    // Change audio source to the mapped folder
                    if (audioManager.changeAudioSource(musicPath.c_str())) {
                        Serial.printf("[RFID-AUDIO] Audio source changed to %s\n", musicPath.c_str());
                        if (audioManager.restartFromFirstFile()) {
                            Serial.println("[RFID-AUDIO] Audio started successfully");
                        } else {
                            Serial.printf("[RFID-AUDIO] Failed to start audio: %s\n", audioManager.getLastError());
                        }
                    } else {
                        Serial.printf("[RFID-AUDIO] Failed to change audio source to %s: %s\n", musicPath.c_str(), audioManager.getLastError());
                    }
                } else {
                    Serial.printf("[RFID-AUDIO] No mapping found for UID: %s - flashing red LED\n", uid);
                    
                    // Flash red LED 3 times for unknown RFID card
                    flashRedLED(3, 200, 150);
                }
            } else if (isSameTag) {
                Serial.printf("[RFID-AUDIO] Same tag re-inserted: %s - Toggling audio playback\n", uid);
                if (audioManager.isPlaying()) {
                    if (audioManager.pausePlayback()) {
                        Serial.println("[RFID-AUDIO] Audio paused");
                    } else {
                        Serial.printf("[RFID-AUDIO] Failed to pause audio: %s\n", audioManager.getLastError());
                    }
                } else {
                    if (audioManager.resumePlayback()) {
                        Serial.println("[RFID-AUDIO] Audio resumed");
                    } else {
                        Serial.printf("[RFID-AUDIO] Failed to resume audio: %s\n", audioManager.getLastError());
                    }
                }
            }
        } else {
            Serial.println("[RFID-AUDIO] Tag removed - Pausing audio playback");
            if (audioManager.isPlaying()) {
                if (audioManager.pausePlayback()) {
                    Serial.println("[RFID-AUDIO] Audio paused due to tag removal");
                } else {
                    Serial.printf("[RFID-AUDIO] Failed to pause audio: %s\n", audioManager.getLastError());
                }
            }
        }
    });
    
    // Enable RFID audio control
    rfidManager.enableAudioControl(true);
    Serial.println("[RFID-AUDIO] RFID audio control enabled");
    
    // Initialize Battery Manager
    Serial.println("Initializing Battery Manager...");
    if (!batteryManager.begin()) {
        Serial.println("Failed to initialize Battery Manager!");
        Serial.println("Continuing without battery monitoring...");
    } else {
        Serial.println("Battery Manager initialized successfully!");
    }
    
    // Initialize Audio Manager
    Serial.println("Initializing Audio Manager...");
    Serial.printf("Audio Manager Mode: %s\n", 
                  (audioManager.getFileSelectionMode() == FileSelectionMode::BUILTIN) ? "BUILTIN" : "CUSTOM");
    
    if (!audioManager.begin()) {
        Serial.println("Failed to initialize Audio Manager!");
        Serial.printf("Audio Manager Error: %s\n", audioManager.getLastError());
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    
    // Print Audio Manager status after initialization
    Serial.println("Audio Manager initialized successfully!");
    audioManager.printAudioStatus();
    
    // RFID will control audio playback - don't start automatically
    Serial.println("Audio Manager ready - waiting for RFID tag to start playback");
    
    // Initialize Button Manager
    Serial.println("Initializing Button Manager...");
    if (!buttonManager.begin()) {
        Serial.println("Failed to initialize Button Manager!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    
    Serial.println("Button Manager initialized");
    
    // Initialize Settings Manager
    Serial.println("Initializing Settings Manager...");
    if (!settingsManager.begin()) {
        Serial.println("Failed to initialize Settings Manager!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    
    // Initialize Rotary Encoder
    Serial.println("Initializing Rotary Encoder...");
    if (!rotaryManager.begin()) {
        Serial.println("Failed to initialize Rotary Encoder!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    
    // Set initial volume from settings
    if (settingsManager.isSettingsLoaded()) {
        float defaultVolume = settingsManager.getDefaultVolume();
        rotaryManager.setVolume(defaultVolume);
        Serial.printf("Volume set from settings: %.2f\n", defaultVolume);
    } else {
        // Fallback to default if settings not loaded
        float defaultVolume = 0.35f; // 35% (matching LOCKED_VOLUME from working script)
        rotaryManager.setVolume(defaultVolume);
        Serial.printf("Volume set to fallback default: %.2f\n", defaultVolume);
    }
    
    // Enable conservative mode to prevent encoder skipping
    rotaryManager.setConservativeMode(true);
    
    // Set up volume control callback to sync with audio manager
    rotaryManager.setVolumeChangeCallback([](float newVolume) {
        // Update audio manager volume
        audioManager.setVolume(newVolume);
        Serial.printf("Volume changed to: %.2f (synced with audio)\n", newVolume);
    });
    
    Serial.println("Setup complete! Ready to play audio.");
    
    // Success - turn LED green
    leds[0] = CRGB::Green;
    FastLED.show();
    
    delay(2000); // Keep green for 2 seconds
}