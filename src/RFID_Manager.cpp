#include "RFID_Manager.h"

// Constructor
RFID_Manager::RFID_Manager(uint8_t sclk, uint8_t miso, uint8_t mosi, uint8_t ss) 
    : mfrc522(ss, MFRC522_RST_PIN) {
    sclk_pin = sclk;
    miso_pin = miso;
    mosi_pin = mosi;
    ss_pin = ss;
    
    initialized = false;
    tagPresent = false;
    lastDetectedUIDSize = 0;
    lastDetectedUIDString = "";
    debounceCounter = 0;
    lastTagCheck = 0;
    audioCallback = nullptr;
    audioControlEnabled = false;
}

// Initialize RFID system
bool RFID_Manager::begin() {
    return begin(true); // Default to self-test enabled
}

// Initialize RFID system with optional self-test
bool RFID_Manager::begin(bool enableSelfTest) {
    if (initialized) {
        Serial.println("RFID_Manager: Already initialized");
        return true;
    }
    
    Serial.println("Initializing RFID MFRC522...");
    Serial.printf("RFID_Manager: SPI pins - SCLK:%d, MISO:%d, MOSI:%d, SS:%d\n", 
                  sclk_pin, miso_pin, mosi_pin, ss_pin);
    
    // Initialize SPI for RFID
    SPI.begin(sclk_pin, miso_pin, mosi_pin, ss_pin);
    
    // Initialize MFRC522
    mfrc522.PCD_Init();
    
    // Optional self-test
    if (enableSelfTest) {
        Serial.println("RFID_Manager: Performing self-test...");
        if (!mfrc522.PCD_PerformSelfTest()) {
            Serial.println("RFID_Manager: MFRC522 self-test failed!");
            Serial.println("RFID_Manager: Continuing without self-test...");
        } else {
            Serial.println("RFID_Manager: MFRC522 self-test passed!");
        }
    } else {
        Serial.println("RFID_Manager: Skipping self-test");
    }
    
    Serial.println("RFID MFRC522 initialized successfully!");
    Serial.println("Scan an RFID card to see the UID!");
    
    initialized = true;
    return true;
}

// Helper function to compare UIDs
bool RFID_Manager::compareUid(const byte* uid, const byte* candidate, byte len) {
    for (byte i = 0; i < len; i++) {
        if (uid[i] != candidate[i]) return false;
    }
    return true;
}

// Convert UID to string representation
String RFID_Manager::uidToString(const byte* uid, byte len) {
    String uidString = "";
    for (byte i = 0; i < len; i++) {
        if (uid[i] < 0x10) uidString += "0";
        uidString += String(uid[i], HEX);
        if (i != len - 1) uidString += ":";
    }
    return uidString;
}

// Set audio control callback
void RFID_Manager::setAudioControlCallback(AudioControlCallback callback) {
    audioCallback = callback;
    Serial.println("[RFID] Audio control callback set");
}

// Main update function - implements MicroPython logic
void RFID_Manager::update() {
    if (!initialized) return;
    
    unsigned long currentMillis = millis();
    
    // Check for RFID cards periodically (every 100ms like MicroPython)
    if (currentMillis - lastTagCheck >= TAG_CHECK_INTERVAL_MS) {
        lastTagCheck = currentMillis;
        
        // Check for tag presence
        bool currentlyDetected = mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial();
        
        if (currentlyDetected) {
            // Tag detected - reset debounce counter
            debounceCounter = 0;
            
            // Convert current UID to string for comparison
            String currentUIDString = uidToString(mfrc522.uid.uidByte, mfrc522.uid.size);
            
            // Check if this is the same tag as before (even if tag was previously removed)
            bool isSameTag = false;
            if (lastDetectedUIDSize > 0 && mfrc522.uid.size == lastDetectedUIDSize) {
                isSameTag = compareUid(mfrc522.uid.uidByte, lastDetectedUID, mfrc522.uid.size);
            }
            
            if (!isSameTag) {
                // New or different tag detected
                Serial.println("Sending UID to audio control\n");
                
                // Store the newly detected tag details
                memcpy(lastDetectedUID, mfrc522.uid.uidByte, mfrc522.uid.size);
                lastDetectedUIDSize = mfrc522.uid.size;
                lastDetectedUIDString = currentUIDString;
                tagPresent = true;
                
                Serial.printf("[RFID] New tag detected: %s\n", currentUIDString.c_str());
                
                // Call audio control callback if enabled
                if (audioControlEnabled && audioCallback) {
                    audioCallback(currentUIDString.c_str(), true, true, false);
                }
            } else if (!tagPresent) {
                // Same tag re-inserted (was previously removed)
                Serial.printf("[RFID] Same tag re-inserted: %s\n", currentUIDString.c_str());
                
                // Call audio control callback for resume/pause logic
                if (audioControlEnabled && audioCallback) {
                    audioCallback(currentUIDString.c_str(), true, false, true);
                }
                
                // Update tag presence state
                tagPresent = true;
            }
            // If same tag is already present (tagPresent = true), do nothing
            
        } else {
            // No tag detected
            debounceCounter++;
            
            if (debounceCounter >= DEBOUNCE_THRESHOLD) {
                // No card detected for DEBOUNCE_THRESHOLD iterations
                if (tagPresent) {
                    Serial.println("No card detected, resetting tag state\n");
                    
                    // Call audio control callback for tag removal
                    if (audioControlEnabled && audioCallback) {
                        audioCallback("", false, false, false);
                    }
                    
                    // Reset tag presence but keep UID in memory for re-insertion detection
                    tagPresent = false;
                    // Don't clear lastDetectedUID, lastDetectedUIDSize, or lastDetectedUIDString
                    // This allows us to recognize when the same tag is re-inserted
                }
                
                debounceCounter = 0;
            }
        }
    }
}

// Print current RFID status
void RFID_Manager::printStatus() {
    if (!initialized) {
        Serial.println("[RFID] Manager not initialized");
        return;
    }
    
    if (tagPresent) {
        Serial.print("[RFID] Tag present - UID: ");
        printTagUID();
        Serial.printf(" (Audio control: %s)\n", audioControlEnabled ? "ENABLED" : "DISABLED");
    } else {
        Serial.printf("[RFID] No tag present (Audio control: %s)\n", audioControlEnabled ? "ENABLED" : "DISABLED");
    }
}

// Print the stored tag UID
void RFID_Manager::printTagUID() {
    if (lastDetectedUIDSize == 0) {
        Serial.print("None");
        return;
    }
    
    Serial.print(lastDetectedUIDString);
}
