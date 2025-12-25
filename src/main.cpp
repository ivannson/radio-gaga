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
#include "SdScanner.h"
#include "MappingStore.h"
#include "WebSetupServer.h"
#include "Logger.h"
#include <WiFi.h>

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

// External speaker amplifier (PAM8302A) shutdown control
// SD pin: HIGH = speaker ON, LOW = speaker OFF (shutdown)
#define SPEAKER_SD_PIN 0

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

// Setup components
SdScanner sdScanner;
MappingStore mappingStore;
WebSetupServer webSetupServer;

// Forward declaration for external triggers (e.g., config button) to start the captive portal
static void startCaptivePortal();

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
  LOG_DEBUG("[HP-DET] applyRoute called with hp=%s", hp ? "true" : "false");
  
  // Disable speaker when headphones are present, enable when they're not
  bool speakerShouldBeEnabled = !hp;
  LOG_DEBUG("[HP-DET] Speaker should be %s", speakerShouldBeEnabled ? "ENABLED" : "DISABLED");

  // Control external speaker amplifier shutdown (PAM8302A on SPEAKER_SD_PIN)
  digitalWrite(SPEAKER_SD_PIN, speakerShouldBeEnabled ? HIGH : LOW);
  LOG_DEBUG("[HP-DET] PAM8302A SD (GPIO%d) set to %s", SPEAKER_SD_PIN,
            speakerShouldBeEnabled ? "HIGH (ON)" : "LOW (SHUTDOWN)");

  // Also set speaker volume to 0 when disabled to ensure it's really off
  if (!speakerShouldBeEnabled) {
    dacManager.setSpeakerVolume(0);
    LOG_DEBUG("[HP-DET] Speaker volume set to 0 for complete muting");
  } else {
    audioManager.pausePlayback();  // Pause audio playback when switching routes
    delay(250);  // Small delay to ensure audio stops before changing route
    // Restore speaker volume to default when enabling speaker output
    dacManager.setSpeakerVolume(6); // Restore normal volume
    LOG_DEBUG("[HP-DET] Speaker volume restored to 6");
  }

  dacManager.enableSpeaker(speakerShouldBeEnabled);
  
  LOG_INFO("[HP-DET] ROUTE -> %s (Speaker: %s)", 
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
      LOG_DEBUG("[HP-DET] GPIO33: %d -> %s (Starting count: %d/3)", 
                digitalRead(HP_GPIO_PIN), 
                gpioReading ? "HEADPHONES" : "SPEAKER",
                g_consecutiveCount);
    } else if (gpioReading == g_targetHPState && g_consecutiveCount < HP_CONSECUTIVE_READS) {
      // Continue counting if same state and not yet reached 3
      g_consecutiveCount++;
      LOG_DEBUG("[HP-DET] Consecutive count: %d/3 for %s", 
                g_consecutiveCount,
                g_targetHPState ? "HEADPHONES" : "SPEAKER");
    }
    
    // Update previous reading
    g_lastGPIOReading = gpioReading;
    
    // If we have enough consecutive readings, apply the change
    if (g_consecutiveCount >= HP_CONSECUTIVE_READS && g_targetHPState != g_currentHPState) {
      g_currentHPState = g_targetHPState;
      applyRoute(g_currentHPState);
      LOG_INFO("[HP-DET] State confirmed: %s after %d consecutive readings", 
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
    LOG_INFO("[HP-DET] Initial route: %s (GPIO33: %d)", 
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
  LOG_DEBUG("handleButtonPress called with button: %d", buttonType);
  LOG_DEBUG("Audio Manager initialized: %s", audioManager.isInitialized() ? "Yes" : "No");

  // Only require a tag for transport controls, never for the encoder button
    auto requiresTag = [](ButtonType b) {
        return b == BUTTON_PREVIOUS || b == BUTTON_NEXT || b == BUTTON_PLAY_PAUSE;
    };

    const bool tagPresent = rfidManager.isTagPresent();
    const bool audioActive = audioManager.isPlaying();

    // Allow transport buttons while audio is already playing even if tag briefly drops out
    if (requiresTag(buttonType) && !tagPresent && !audioActive) {
        LOG_WARN("No RFID tag present - transport buttons disabled");
        return;
    }

    if (tagPresent) {
        LOG_DEBUG("RFID tag present: %s - buttons enabled",
                            rfidManager.getLastDetectedUIDString().c_str());
    } else if (audioActive) {
        LOG_INFO("Tag not detected, but audio is active - allowing transport control");
    }

  switch (buttonType) {
    case BUTTON_PREVIOUS:
      LOG_INFO("Previous button pressed - going to previous track");
      if (audioManager.isInitialized()) {
        if (!audioManager.playPreviousFile()) {
          LOG_ERROR("Failed to play previous track: %s", audioManager.getLastError());
        }
      } else {
        LOG_ERROR("Audio Manager not initialized, cannot play previous track");
      }
      break;

    case BUTTON_PLAY_PAUSE:
      LOG_INFO("Play/Pause button pressed - toggling playback");
      if (audioManager.isInitialized()) {
        audioManager.updatePlaybackState();
        if (audioManager.isPlaying()) {
          if (!audioManager.pausePlayback()) {
            LOG_ERROR("Failed to pause: %s", audioManager.getLastError());
          }
        } else {
          if (!audioManager.resumePlayback()) {
            LOG_ERROR("Failed to resume: %s", audioManager.getLastError());
          }
        }
      } else {
        LOG_ERROR("Audio Manager not initialized, cannot toggle playback");
      }
      break;

    case BUTTON_NEXT:
      LOG_INFO("Next button pressed - going to next track");
      if (audioManager.isInitialized()) {
        if (!audioManager.playNextFile()) {
          LOG_ERROR("Failed to play next track: %s", audioManager.getLastError());
        }
      } else {
        LOG_ERROR("Audio Manager not initialized, cannot play next track");
      }
      break;

    case BUTTON_ENCODER:
      LOG_INFO("Encoder button pressed - (always active)");
      // No action bound; web setup is triggered via long-press in loop
      break;

    default:
      LOG_WARN("Unknown button type: %d", buttonType);
      break;
  }
}
// ============================================================================
// DEBUG FUNCTIONS
// ============================================================================

void listAllSDContents(const char* path, int depth = 0) {
    File root = SD_MMC.open(path);
    if (!root) {
        LOG_DEBUG("Failed to open: %s", path);
        return;
    }
    
    if (!root.isDirectory()) {
        LOG_DEBUG("Not a directory: %s", path);
        root.close();
        return;
    }
    
    // Print current directory
    String indent = "";
    for (int i = 0; i < depth; i++) {
        indent += "  ";
    }
    LOG_DEBUG("%s📁 %s/", indent.c_str(), path);
    
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
            
            LOG_DEBUG("%s📄 %s (%s)", fileIndent.c_str(), filename.c_str(), sizeStr.c_str());
        }
        
        file.close();
        file = root.openNextFile();
    }
    
    root.close();
}

// Convenience helper to start the captive portal/web setup from other triggers
static void startCaptivePortal() {
    if (webSetupServer.isActive()) {
        LOG_INFO("Captive portal already active");
        return;
    }
    LOG_INFO("Starting captive portal (web setup) on demand");
    if (!webSetupServer.start()) {
        LOG_ERROR("Failed to start Web Setup server");
    }
}

// ============================================================================
// MAIN LOOP FUNCTION
// ============================================================================

void loop() {
    static bool prevWebSetupActive = false;
    static uint32_t lastWebSetupStopMs = 0;
    static bool webSetupJustStopped = false;
    
    // Update button states
    buttonManager.update();
    
    // Update RFID detection (needed for both normal mode and setup mode)
    static uint32_t lastRFID = 0;
    if (millis() - lastRFID >= 100) { // 10 Hz max
        rfidManager.update();
        lastRFID = millis();
    }

    // If web setup is active, serve HTTP and skip normal player logic
    const bool webSetupActive = webSetupServer.isActive();
    if (webSetupActive) {
        webSetupServer.loop();
        prevWebSetupActive = true;
        webSetupJustStopped = false;
        return;
    }
    if (prevWebSetupActive && !webSetupActive) {
        lastWebSetupStopMs = millis();
        prevWebSetupActive = false;
        webSetupJustStopped = true;
        // Small yield to avoid immediate re-entry on same loop iteration
        delay(10);
    }
    
    // Enter setup mode on encoder long press release
    if (buttonManager.getButtonState() == BUTTON_RELEASED_LONG &&
        buttonManager.getLastButton() == BUTTON_ENCODER) {
        // Prevent immediate re-entry right after stopping via web exit
        if (webSetupJustStopped || millis() - lastWebSetupStopMs < 2000) {
            return;
        }
        LOG_INFO("Encoder long press detected - starting Web Setup server");
        if (!webSetupServer.start()) {
            LOG_ERROR("Failed to start Web Setup server");
        }
        return; // Skip normal player logic this cycle
    }
    webSetupJustStopped = false;

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
                LOG_DEBUG("New button pressed: %d", currentButton);
            }
            
            // Only process if button was just pressed (not held)
            if (!buttonProcessed && buttonManager.isButtonPressed(currentButton)) {
                LOG_DEBUG("Processing button press: %d", currentButton);
                handleButtonPress(currentButton);
                buttonProcessed = true;
            }
        } else {
            // No button pressed - reset state
            if (lastButtonState != BUTTON_NONE) {
                lastButtonState = BUTTON_NONE;
                buttonProcessed = false;
                LOG_DEBUG("Button released");
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

    // Handle audio playback after controls to keep UI responsive even if copy() runs long
    if (audioManager.isInitialized()) {
        audioManager.update();
    }
    
    // Debug: Print status occasionally
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 5000) { // Every 5 seconds
        //Serial.printf("Encoder status: value=%d, volume=%.2f\n", 
        //            rotaryManager.getEncoderValue(), rotaryManager.getVolume());
        
        // Print audio status
        if (audioManager.isInitialized()) {
            LOG_DEBUG("Audio: %s, File: %s", 
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
    
    // Initialize logging system
    initLogger(LogLevel::INFO);  // Set to DEBUG for development, INFO for normal operation
    
    // Reset system
    delay(100);

    // Keep WiFi off until AP setup is explicitly started
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    LOG_INFO("WiFi disabled at startup; AP will enable WiFi on demand");

    // Initialize WLED
    FastLED.addLeds<WS2812B, WLED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
    
    // Start with LED off
    leds[0] = CRGB::Black;
    FastLED.show();

    // Enable internal pull-ups for 4-bit mode pins (from your working script)
    gpio_pullup_en(GPIO_NUM_2);   // DAT0
    gpio_pullup_en(GPIO_NUM_15);  // CMD
    gpio_pullup_dis(GPIO_NUM_14);  // CLK
    
    // Configure external speaker amplifier SD pin (PAM8302A)
    pinMode(SPEAKER_SD_PIN, OUTPUT);
    digitalWrite(SPEAKER_SD_PIN, LOW); // start with speaker amplifier in shutdown to avoid pops
    LOG_INFO("PAM8302A SD (GPIO%d) configured as OUTPUT, initial state SHUTDOWN (LOW)", SPEAKER_SD_PIN);
    
    // Configure GPIO33 as input for headphone detection
    pinMode(33, INPUT_PULLUP);
    LOG_INFO("GPIO33 configured as input with pull-up for headphone detection");

    // Initialize SD Manager first (needed for audio)
    LOG_INFO("Initializing SD Manager...");
    if (!sdManager.begin()) {
        LOG_ERROR("Failed to initialize SD Manager!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    sdCardMounted = true;

    // DEBUG: List all files and directories on SD card
    LOG_DEBUG("=== DEBUG: SD CARD CONTENTS ===");
    listAllSDContents("/");
    LOG_DEBUG("=== END DEBUG: SD CARD CONTENTS ===");

    // Initialize DAC Manager
    LOG_INFO("Initializing DAC Manager...");
    if (!dacManager.begin()) {
        LOG_ERROR("Failed to initialize DAC Manager!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    dacInitialized = true;
    
    // Configure DAC with proper volumes
    LOG_INFO("Configuring DAC...");
    if (!dacManager.configure(true, true, 6, 6)) {  // headphone_detection, speaker_output, hp_vol, spk_vol
        LOG_ERROR("Failed to configure DAC!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    
    // Explicitly disable speaker at startup to ensure known state
    LOG_INFO("Explicitly disabling speaker at startup...");
    dacManager.enableSpeaker(false);
    dacManager.setSpeakerVolume(0);
    LOG_INFO("Speaker disabled and muted at startup");
    
    // Wait for DAC and headphone detection to stabilize
    LOG_INFO("Waiting for DAC and headphone detection to stabilize...");
    delay(500);  // Give headphone detection circuit time to settle
    
    // Default to speaker OFF until we read a real status
    dacManager.enableSpeaker(false);

    // Give the jack-detect block a moment to settle
    delay(150);
    
    // First routing decision using simple GPIO-based headphone detection system
    updateOutputRoute(true);
    
    // Initialize RFID Manager
    if (!rfidManager.begin(true)) {  // Enable self-test
        LOG_ERROR("Failed to initialize RFID Manager");
        return;
    }
    LOG_INFO("RFID MFRC522 initialized successfully!");

    // Initialize storage components for mapping
    if (!sdScanner.begin(SD_MMC)) {
        LOG_ERROR("Failed to initialize SD Scanner");
        return;
    }
    
    if (!mappingStore.begin(SD_MMC, "/lookup.ndjson")) {
        LOG_ERROR("Failed to initialize Mapping Store");
        return;
    }
    
    // Set up RFID audio control callback
    rfidManager.setAudioControlCallback([](const char* uid, bool tagPresent, bool isNewTag, bool isSameTag) {
        // Suppress audio control during web setup
        if (webSetupServer.isActive()) {
            LOG_DEBUG("[RFID-AUDIO] Web setup active - audio control suppressed");
            return;
        }
        
        if (tagPresent) {
            if (isNewTag) {
                LOG_INFO("[RFID-AUDIO] New tag detected: %s - Looking up music folder", uid);
                
                // Look up the music folder path for this UID
                String musicPath;
                if (mappingStore.getPathFor(uid, musicPath)) {
                    LOG_INFO("[RFID-AUDIO] Found mapping: %s -> %s", uid, musicPath.c_str());
                    
                    // Change audio source to the mapped folder
                    if (audioManager.changeAudioSource(musicPath.c_str())) {
                        LOG_INFO("[RFID-AUDIO] Audio source changed to %s", musicPath.c_str());
                        if (audioManager.restartFromFirstFile()) {
                            LOG_INFO("[RFID-AUDIO] Audio started successfully");
                        } else {
                            LOG_ERROR("[RFID-AUDIO] Failed to start audio: %s", audioManager.getLastError());
                        }
                    } else {
                        LOG_ERROR("[RFID-AUDIO] Failed to change audio source to %s: %s", musicPath.c_str(), audioManager.getLastError());
                    }
                } else {
                    LOG_WARN("[RFID-AUDIO] No mapping found for UID: %s - flashing red LED", uid);
                    
                    // Flash red LED 3 times for unknown RFID card
                    flashRedLED(3, 200, 150);
                }
            } else if (isSameTag) {
                LOG_INFO("[RFID-AUDIO] Same tag re-inserted: %s - Toggling audio playback", uid);
                if (audioManager.isPlaying()) {
                    if (audioManager.pausePlayback()) {
                        LOG_INFO("[RFID-AUDIO] Audio paused");
                    } else {
                        LOG_ERROR("[RFID-AUDIO] Failed to pause audio: %s", audioManager.getLastError());
                    }
                } else {
                    if (audioManager.resumePlayback()) {
                        LOG_INFO("[RFID-AUDIO] Audio resumed");
                    } else {
                        LOG_ERROR("[RFID-AUDIO] Failed to resume audio: %s", audioManager.getLastError());
                    }
                }
            }
        } else {
            LOG_INFO("[RFID-AUDIO] Tag removed - Pausing audio playback");
            if (audioManager.isPlaying()) {
                if (audioManager.pausePlayback()) {
                    LOG_INFO("[RFID-AUDIO] Audio paused due to tag removal");
                } else {
                    LOG_ERROR("[RFID-AUDIO] Failed to pause audio: %s", audioManager.getLastError());
                }
            }
        }
    });
    
    // Enable RFID audio control
    rfidManager.enableAudioControl(true);
    LOG_INFO("[RFID-AUDIO] RFID audio control enabled");
    
    // Initialize Battery Manager
    LOG_INFO("Initializing Battery Manager...");
    if (!batteryManager.begin()) {
        LOG_WARN("Failed to initialize Battery Manager!");
        LOG_WARN("Continuing without battery monitoring...");
    } else {
        LOG_INFO("Battery Manager initialized successfully!");
    }
    
    // Initialize Audio Manager
    LOG_INFO("Initializing Audio Manager...");
    LOG_INFO("Audio Manager Mode: %s", 
             (audioManager.getFileSelectionMode() == FileSelectionMode::CUSTOM) ? "CUSTOM" : "BUILTIN");
    
    if (!audioManager.begin()) {
        LOG_ERROR("Failed to initialize Audio Manager!");
        LOG_ERROR("Audio Manager Error: %s", audioManager.getLastError());
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    
    // Print Audio Manager status after initialization
    LOG_INFO("Audio Manager initialized successfully!");
    audioManager.printAudioStatus();
    
    // RFID will control audio playback - don't start automatically
    LOG_INFO("Audio Manager ready - waiting for RFID tag to start playback");
    
    // Initialize Button Manager
    LOG_INFO("Initializing Button Manager...");
    if (!buttonManager.begin()) {
        LOG_ERROR("Failed to initialize Button Manager!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    
    LOG_INFO("Button Manager initialized");
    
    // Initialize Settings Manager
    LOG_INFO("Initializing Settings Manager...");
    if (!settingsManager.begin()) {
        LOG_ERROR("Failed to initialize Settings Manager!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }

    // Initialize Web Setup server (open AP, starts on-demand) after settings/battery are ready
    if (!webSetupServer.begin(&mappingStore, &sdScanner, &rfidManager, "/", &settingsManager, &batteryManager)) {
        LOG_ERROR("Failed to initialize Web Setup server");
    }
    LOG_INFO("Scan an RFID card to see the UID!");

    // Determine initial volume from settings (or fallback) and
    // apply it directly to the Audio Manager so it is effective
    // even before any encoder movement.
    float initialVolume = 0.35f; // Fallback default
    if (settingsManager.isSettingsLoaded()) {
        initialVolume = settingsManager.getDefaultVolume();
        LOG_INFO("Initial volume loaded from settings: %.2f", initialVolume);
    } else {
        LOG_INFO("Settings not loaded, using fallback initial volume: %.2f", initialVolume);
    }

    // Apply initial volume directly to the audio pipeline
    audioManager.setVolume(initialVolume);
    LOG_INFO("Audio Manager volume set to initial value: %.2f", initialVolume);
    
    // Initialize Rotary Encoder
    LOG_INFO("Initializing Rotary Encoder...");
    if (!rotaryManager.begin()) {
        LOG_ERROR("Failed to initialize Rotary Encoder!");
        leds[0] = CRGB::Red;
        FastLED.show();
        while(1) delay(1000);
    }
    
    // Set up volume control callback to sync with audio manager
    rotaryManager.setVolumeChangeCallback([](float newVolume) {
        // Update audio manager volume
        audioManager.setVolume(newVolume);
        LOG_DEBUG("Volume changed to: %.2f (synced with audio)", newVolume);
    });
    
    // Sync rotary encoder position and internal volume with the
    // already-applied initial volume.
    rotaryManager.setVolume(initialVolume);
    LOG_INFO("Rotary encoder volume initialized to: %.2f", initialVolume);
    
    // Enable conservative mode to prevent encoder skipping
    rotaryManager.setConservativeMode(true);
    
    LOG_INFO("Setup complete! Ready to play audio.");
    
    // Success - turn LED green
    leds[0] = CRGB::Green;
    FastLED.show();
    
    delay(2000); // Keep green for 2 seconds
}