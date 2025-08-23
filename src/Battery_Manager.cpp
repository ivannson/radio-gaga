#include "Battery_Manager.h"

// Constructor
Battery_Manager::Battery_Manager(uint8_t sda, uint8_t scl)
    : initialized(false), sdaPin(sda), sclPin(scl),
      batteryVoltage(0.0f), batteryPercentage(0.0f),
      lastReadingTime(0) {
}

// Initialize battery manager
bool Battery_Manager::begin() {
    if (initialized) return true;

    Serial.printf("Init Fuel Gauge on SDA:%d SCL:%d\n", sdaPin, sclPin);
    Wire.begin(sdaPin, sclPin);
    Wire.setClock(100000);

    // Make sure we bind to THIS Wire instance
    if (!lipo.begin(Wire)) {
        Serial.println("MAX1704x not found at 0x36");
        return false;
    }

    // Hard resync if gauge lost context (battery swapped / power-cycled)
    lipo.reset();        // one-time at boot if readings look wrong
    delay(200);
    lipo.quickStart();   // required after battery connect or reset
    delay(50);

    // Optional: low-SOC alert at 20%
    // lipo.setThreshold(20);

    // Optional: print version (if your lib has it)
    // Serial.printf("MAX1704x ver: 0x%04X\n", lipo.getVersion());

    update();            // prime first reading
    initialized = true;
    return true;
}

// Test connection to the fuel gauge
bool Battery_Manager::testConnection() {
    // Try to read the version register
    uint16_t version = lipo.getVersion();
    if (version == 0xFFFF) {
        Serial.println("Battery_Manager: Failed to read version register");
        return false;
    }
    
    Serial.printf("Battery_Manager: MAX1704x version: 0x%04X\n", version);
    return true;
}

// Main update function - call this in main loop
void Battery_Manager::update() {
    if (!initialized) return;
    
    unsigned long currentMillis = millis();
    
    // Read battery status every 5 seconds
    if (currentMillis - lastReadingTime >= READING_INTERVAL) {
        lastReadingTime = currentMillis;
        
        // Read voltage using the library
        batteryVoltage = lipo.getVoltage();
        
        // Read SOC using the library
        batteryPercentage = lipo.getSOC();
        Serial.println(batteryPercentage);
        
        // Print battery status
        printBatteryStatus();
    }
}

// Print battery status
void Battery_Manager::printBatteryStatus() {
    if (!initialized) {
        Serial.println("[BATTERY] Manager not initialized");
        return;
    }
    
    Serial.printf("[BATTERY] Voltage: %.3fV (%.1f%%)\n",
                  batteryVoltage, batteryPercentage);
}

// Set I2C pins
void Battery_Manager::setI2CPins(uint8_t sda, uint8_t scl) {
    if (initialized) {
        Serial.println("Battery_Manager: Cannot change pins after initialization");
        return;
    }
    
    sdaPin = sda;
    sclPin = scl;
    Serial.printf("Battery_Manager: I2C pins set to SDA:%d, SCL:%d\n", sdaPin, sclPin);
}
