#include "DAC_Manager.h"
#include <Wire.h>

// Constructor
DAC_Manager::DAC_Manager(uint8_t reset_pin, uint8_t sda_pin, uint8_t scl_pin, uint8_t address) {
    resetPin = reset_pin;
    sdaPin = sda_pin;
    sclPin = scl_pin;
    i2cAddress = address;
    initialized = false;
    
    // Default configuration
    // Remove headphone detection - now handled in main.cpp
    // enableHeadphoneDetection = true;
    enableSpeakerOutput = true;
    defaultHeadphoneVolume = 6;
    defaultSpeakerVolume = 0;
}

// Initialize the DAC
bool DAC_Manager::begin() {
    Serial.println("Initializing TLV320DAC3100...");
    
    // Initialize I2C first
    Serial.println("Initializing I2C...");
    Wire.begin(sdaPin, sclPin);
    Serial.println("I2C initialized");
    delay(100);
    
    // Reset DAC
    reset();
    
    // Check I2C communication
    if (!checkI2CCommunication()) {
        Serial.println("DAC not responding on I2C bus!");
        return false;
    }
    
    // Initialize DAC
    Serial.println("Calling codec.begin()...");
    if (!codec.begin()) {
        Serial.println("Failed to initialize TLV320DAC3100!");
        return false;
    }
    
    Serial.println("codec.begin() successful");
    initialized = true;
    return true;
}

// Reset DAC
void DAC_Manager::reset() {
    Serial.println("Resetting DAC...");
    Serial.printf("Setting pin %d as output...\n", resetPin);
    pinMode(resetPin, OUTPUT);
    Serial.println("Pin mode set");
    delay(50);
    
    Serial.println("Setting pin LOW...");
    digitalWrite(resetPin, LOW);
    Serial.println("Pin set LOW");
    delay(100);
    
    Serial.println("Setting pin HIGH...");
    digitalWrite(resetPin, HIGH);
    Serial.println("Pin set HIGH");
    delay(100);
    
    Serial.println("DAC reset complete");
    delay(100);
}

// Check I2C communication
bool DAC_Manager::checkI2CCommunication() {
    Serial.println("Checking I2C communication...");
    Wire.beginTransmission(i2cAddress);
    byte error = Wire.endTransmission();
    Serial.printf("I2C test to address 0x%02X: error = %d\n", i2cAddress, error);
    
    if (error != 0) {
        Serial.println("DAC not responding on I2C bus!");
        Serial.println("Possible issues:");
        Serial.println("1. DAC not powered");
        Serial.println("2. Wrong I2C address");
        Serial.println("3. Wiring issue (SDA/SCL)");
        Serial.println("4. DAC not properly reset");
        return false;
    }
    
    Serial.println("I2S communication OK");
    return true;
}

// Basic configuration (minimal setup)
bool DAC_Manager::configureBasic() {
    if (!initialized) {
        Serial.println("DAC not initialized!");
        return false;
    }
    
    Serial.println("Basic DAC configuration...");
    
    // Set I2S interface
    codec.setCodecInterface(TLV320DAC3100_FORMAT_I2S, TLV320DAC3100_DATA_LEN_16);
    Serial.println("Interface set");
    delay(50);
    
    // Set clock configuration
    codec.setCodecClockInput(TLV320DAC3100_CODEC_CLKIN_PLL);
    Serial.println("Clock input set");
    delay(50);
    
    codec.setPLLClockInput(TLV320DAC3100_PLL_CLKIN_BCLK);
    Serial.println("PLL clock set");
    delay(50);
    
    codec.setPLLValues(1, 2, 32, 0);
    Serial.println("PLL values set");
    delay(50);
    
    // Set DAC clocks
    codec.setNDAC(true, 8);
    codec.setMDAC(true, 2);
    codec.powerPLL(true);
    Serial.println("DAC clocks configured");
    delay(50);
    
    Serial.println("DAC basic configuration complete!");
    return true;
}

// Full configuration (all features)
bool DAC_Manager::configureFull() {
    if (!initialized) {
        Serial.println("DAC not initialized!");
        return false;
    }
    
    Serial.println("Full DAC configuration...");
    
    // Basic configuration first
    if (!configureBasic()) {
        return false;
    }
    
    // Configure data path
    codec.setDACDataPath(true, true, TLV320_DAC_PATH_NORMAL, TLV320_DAC_PATH_NORMAL, TLV320_VOLUME_STEP_1SAMPLE);
    Serial.println("Data path configured");
    delay(50);
    
    // Configure analog inputs
    codec.configureAnalogInputs(TLV320_DAC_ROUTE_MIXER, TLV320_DAC_ROUTE_MIXER, false, false, false, false);
    Serial.println("Analog inputs configured");
    delay(50);
    
    // Configure volume control
    codec.setDACVolumeControl(false, false, TLV320_VOL_INDEPENDENT);
    codec.setChannelVolume(false, defaultHeadphoneVolume);
    codec.setChannelVolume(true, defaultHeadphoneVolume);
    Serial.println("Volume control configured");
    delay(50);
    
    // Configure headphone driver
    codec.configureHeadphoneDriver(true, true, TLV320_HP_COMMON_1_35V, false);
    codec.configureHPL_PGA(0, true);
    codec.configureHPR_PGA(0, true);
    codec.setHPLVolume(true, defaultHeadphoneVolume);
    codec.setHPRVolume(true, defaultHeadphoneVolume);
    Serial.println("Headphone driver configured");
    delay(50);
    
    // Configure speaker
    if (enableSpeakerOutput) {
        codec.configureSPK_PGA(TLV320_SPK_GAIN_6DB, true);
        codec.setSPKVolume(true, defaultSpeakerVolume);
        Serial.println("Speaker configured");
        delay(50);
    }
    
    // Configure microphone bias - DISABLE during headphone detection
    codec.configMicBias(false, false, TLV320_MICBIAS_2V);
    Serial.println("Mic bias OFF for jack detect");
    delay(10);
    
    // Remove headphone detection configuration - now handled in main.cpp
    // Headphone detection is now configured directly in main.cpp using getCodec()
    
    Serial.println("DAC full configuration complete!");
    
    // Remove sanity check - now handled in main.cpp
    // if (enableHeadphoneDetection) {
    //     Serial.println("Sanity check: First few headphone status reads:");
    //     for (int i = 0; i < 3; i++) {
    //         tlv320_headset_status_t status = codec.getHeadsetStatus();
    //         Serial.printf("  Read %d: %d (%s)\n", i, (int)status,
    //                       status == TLV320_HEADSET_NONE ? "NONE" :
    //                       status == TLV320_HEADSET_WITHOUT_MIC ? "WITHOUT_MIC" :
    //                       status == TLV320_HEADSET_WITH_MIC ? "WITH_MIC" : "UNKNOWN");
    //         if (i < 2) delay(20); // 20ms between reads
    //     }
    // }
    
    return true;
}

// Remove all headphone-related methods - now handled in main.cpp
// tlv320_headset_status_t DAC_Manager::getHeadphoneStatus() { ... }
// bool DAC_Manager::bootProbeHeadphoneDetection() { ... }

// Configure DAC with custom settings
bool DAC_Manager::configure(bool enable_headphone_detection, bool enable_speaker_output, 
                           uint8_t headphone_volume, uint8_t speaker_volume) {
    Serial.printf("DAC_Manager::configure called with: HP_DET=%s, SPK=%s, HP_VOL=%d, SPK_VOL=%d\n",
                  enable_headphone_detection ? "true" : "false",
                  enable_speaker_output ? "true" : "false",
                  headphone_volume, speaker_volume);
    
    // Remove headphone detection handling - now handled in main.cpp
    // enableHeadphoneDetection = enable_headphone_detection;
    enableSpeakerOutput = enable_speaker_output;
    defaultHeadphoneVolume = headphone_volume;
    defaultSpeakerVolume = speaker_volume;
    
    Serial.printf("DAC_Manager: Configuration set - HP_DET=disabled, SPK=%s, HP_VOL=%d, SPK_VOL=%d\n",
                  enableSpeakerOutput ? "true" : "false",
                  defaultHeadphoneVolume, defaultSpeakerVolume);
    
    return configureFull();
}

// Enable/disable speaker output
void DAC_Manager::enableSpeaker(bool enable) {
    if (!initialized) {
        Serial.printf("DAC_Manager::enableSpeaker(%s) called but DAC not initialized\n", enable ? "true" : "false");
        return;
    }
    
    Serial.printf("DAC_Manager::enableSpeaker(%s) - calling codec.enableSpeaker()\n", enable ? "true" : "false");
    codec.enableSpeaker(enable);
    
    // Verify the change took effect
    delay(5); // Small delay for register update
    bool actualState = codec.speakerEnabled();
    Serial.printf("DAC_Manager::enableSpeaker(%s) - codec reports speaker is %s\n", 
                  enable ? "true" : "false", actualState ? "ENABLED" : "DISABLED");
}

// Set headphone volume
void DAC_Manager::setHeadphoneVolume(uint8_t volume) {
    if (!initialized) return;
    codec.setHPLVolume(true, volume);
    codec.setHPRVolume(true, volume);
}

// Set speaker volume
void DAC_Manager::setSpeakerVolume(uint8_t volume) {
    if (!initialized) {
        Serial.printf("DAC_Manager::setSpeakerVolume(%d) called but DAC not initialized\n", volume);
        return;
    }
    
    Serial.printf("DAC_Manager::setSpeakerVolume(%d) - calling codec.setSPKVolume()\n", volume);
    codec.setSPKVolume(true, volume);
    Serial.printf("DAC_Manager::setSpeakerVolume(%d) - volume set\n", volume);
} 