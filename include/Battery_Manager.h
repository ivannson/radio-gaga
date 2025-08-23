#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

class Battery_Manager {
private:
    SFE_MAX1704X lipo; // SparkFun MAX1704x Fuel Gauge object
    bool initialized;
    uint8_t sdaPin;
    uint8_t sclPin;
    
    // Battery readings
    float batteryVoltage;
    float batteryPercentage;
    
    // Timing
    unsigned long lastReadingTime;
    const unsigned long READING_INTERVAL = 5000; // 5 seconds
    
public:
    Battery_Manager(uint8_t sda = 22, uint8_t scl = 21);
    
    // Initialization
    bool begin();
    bool isInitialized() const { return initialized; }
    
    // Main update function - call this in main loop
    void update();
    
    // Battery information
    float getBatteryVoltage() const { return batteryVoltage; }
    float getBatteryPercentage() const { return batteryPercentage; }
    
    // Configuration
    void setI2CPins(uint8_t sda, uint8_t scl);
    
    // Debug and status
    void printBatteryStatus();
    bool testConnection();
};

#endif // BATTERY_MANAGER_H
