#pragma once

#include "../DebugConfig.h"
#include <Arduino.h>
#include "ISensor.h"
#include "BMX280Sensor.h"
#include "AHT20Sensor.h"
#include "SCD4xSensor.h"
#include "SPS30Sensor.h"
#include <Wire.h>

/**
 * @brief Known I2C addresses for supported sensors
 */
namespace I2CAddress {
    const uint8_t BMX280_PRIMARY = 0x76;
    const uint8_t BMX280_SECONDARY = 0x77;
    const uint8_t AHT20 = 0x38;
    const uint8_t SCD4X = 0x62;
    const uint8_t SPS30 = 0x69;
}

/**
 * @brief Manages all sensors and provides unified data access
 */
class SensorManager {
public:
    SensorManager() : _bmx280(nullptr), _aht20(nullptr), _scd4x(nullptr), _sps30(nullptr) {}
    
    ~SensorManager() {
        cleanup();
    }
    
    // Prevent copying (rule of three)
    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;
    
    /**
     * @brief Initialize I2C and detect available sensors
     * @param sdaPin I2C SDA pin
     * @param sclPin I2C SCL pin
     */
    void begin(int sdaPin = 0, int sclPin = 2) {
        Wire.begin(sdaPin, sclPin);
        Wire.setClock(100000);  // 100kHz for better compatibility
        
        // Scan I2C bus
        _i2cAddresses[0] = '\0';
        scanI2C();
        
        // Initialize sensors based on detected addresses
        initializeSensors();
        
        // Log sensor status
        logSensorStatus();
    }
    
    /**
     * @brief Read all available sensors
     */
    void readAll() {
        // Clear previous data to avoid stale values
        _sensorData.clear();
        
        // Determine if we have other temp/humi sensors for SCD4x
        bool hasOtherTempSensor = (_bmx280 && _bmx280->isAvailable()) || 
                                   (_aht20 && _aht20->isAvailable());
        
        if (_bmx280 && _bmx280->isAvailable()) {
            _bmx280->read(_sensorData);
        }
        
        if (_aht20 && _aht20->isAvailable()) {
            _aht20->read(_sensorData);
        }
        
        if (_scd4x && _scd4x->isAvailable()) {
            _scd4x->read(_sensorData, hasOtherTempSensor, hasOtherTempSensor);
        }
        
        if (_sps30 && _sps30->isAvailable()) {
            _sps30->read(_sensorData);
        }
        
        // Add imperial temperature conversions
        addImperialConversions();
        
        _sensorData.garbageCollect();
    }
    
    /**
     * @brief Get sensor data JSON document
     */
    DynamicJsonDocument& getData() { return _sensorData; }
    const DynamicJsonDocument& getData() const { return _sensorData; }
    
    /**
     * @brief Get I2C addresses found during scan
     */
    const char* getI2CAddresses() const { return _i2cAddresses; }
    
    /**
     * @brief Get number of available sensors
     */
    uint8_t getSensorCount() const {
        uint8_t count = 0;
        if (_bmx280 && _bmx280->isAvailable()) count++;
        if (_aht20 && _aht20->isAvailable()) count++;
        if (_scd4x && _scd4x->isAvailable()) count++;
        if (_sps30 && _sps30->isAvailable()) count++;
        return count;
    }
    
    /**
     * @brief Check if we should use remote sensor (no local sensors found)
     */
    bool shouldUseRemoteSensor() const {
        return _i2cAddresses[0] == '\0';
    }
    
    /**
     * @brief Get CO2 value for LED indicator
     * @return CO2 in ppm, or -1 if not available
     */
    float getCO2() const {
        if (_scd4x && _scd4x->isAvailable()) {
            return _scd4x->getLastCO2();
        }
        if (_sensorData.containsKey("co2")) {
            return _sensorData["co2"].as<float>();
        }
        return -1;
    }
    
    /**
     * @brief Get PM2.5 value for LED indicator
     * @return PM2.5 in µg/m³, or -1 if not available
     */
    float getPM25() const {
        if (_sensorData.containsKey("pm25")) {
            return _sensorData["pm25"].as<float>();
        }
        return -1;
    }
    
    /**
     * @brief Check if SCD4x is available
     */
    bool hasSCD4x() const { return _scd4x && _scd4x->isAvailable(); }

    /**
     * @brief Check if BMX280 is available
     */
    bool hasBMX280() const { return _bmx280 && _bmx280->isAvailable(); }
    
    /**
     * @brief Check if AHT20 is available
     */
    bool hasAHT20() const { return _aht20 && _aht20->isAvailable(); }
    
    /**
     * @brief Check if SPS30 is available
     */
    bool hasSPS30() const { return _sps30 && _sps30->isAvailable(); }

    /**
     * @brief Add imperial unit conversions for temperature
     */
    void addImperialConversions() {
        // Convert temp to Fahrenheit
        if (_sensorData.containsKey("temp")) {
            float celsius = _sensorData["temp"].as<float>();
            float fahrenheit = celsiusToFahrenheit(celsius);
            _sensorData["temp_imp"] = fahrenheit;
        }
        
        // Convert temp2 to Fahrenheit
        if (_sensorData.containsKey("temp2")) {
            float celsius = _sensorData["temp2"].as<float>();
            float fahrenheit = celsiusToFahrenheit(celsius);
            _sensorData["temp2_imp"] = fahrenheit;
        }
    }

    /**
     * @brief Add sensor data from external source (remote sensor)
     */
    void addData(const String& key, float value) {
        _sensorData[key] = value;
        
        if (key == "temp" || key == "temp2") {
            float imperial = celsiusToFahrenheit(value);
            _sensorData[key + "_imp"] = imperial;
        }
    }
    
    /**
     * @brief Check if SPS30 sensor is enabled
     * @return True if enabled, false otherwise
     */
    bool isSPS30Enabled() const {
        return _sps30 && _sps30->isEnabled();
    }

    /**
     * @brief Enable or disable SPS30 sensor
     * @param enabled True to enable, false to disable
     */
    void setSPS30Enabled(bool enabled) {
        if (_sps30) {
            _sps30->setEnabled(enabled);
        }
    }

private:
    BMX280Sensor* _bmx280;
    AHT20Sensor* _aht20;
    SCD4xSensor* _scd4x;
    SPS30Sensor* _sps30;
    
    DynamicJsonDocument _sensorData{1024};
    char _i2cAddresses[64];  // Use fixed buffer instead of String
    
    /**
     * @brief Clean up all allocated sensors
     */
    void cleanup() {
        if (_bmx280) { delete _bmx280; _bmx280 = nullptr; }
        if (_aht20) { delete _aht20; _aht20 = nullptr; }
        if (_scd4x) { delete _scd4x; _scd4x = nullptr; }
        if (_sps30) { delete _sps30; _sps30 = nullptr; }
    }
    
    /**
     * @brief Convert Celsius to Fahrenheit
     */
    static float celsiusToFahrenheit(float celsius) {
        return 1.8f * celsius + 32.0f;
    }
    
    /**
     * @brief Check if a specific I2C address was found
     */
    bool hasI2CAddress(uint8_t address) const {
        char addrStr[8];
        snprintf(addrStr, sizeof(addrStr), "0x%02x", address);
        return strstr(_i2cAddresses, addrStr) != nullptr;
    }
    
    void scanI2C() {
        DEBUG_PRINTLN(F("[SensorManager] Scanning I2C bus..."));
        
        uint8_t nDevices = 0;
        size_t offset = 0;
        _i2cAddresses[0] = '\0';
        
        for (uint8_t address = 1; address < 127; address++) {
            Wire.beginTransmission(address);
            uint8_t error = Wire.endTransmission();
            
            if (error == 0) {
                DEBUG_PRINTF("[SensorManager] Found device at 0x%02X\n", address);
                
                // Add to address list with bounds checking
                if (offset > 0 && offset < sizeof(_i2cAddresses) - 6) {
                    _i2cAddresses[offset++] = ',';
                }
                if (offset < sizeof(_i2cAddresses) - 5) {
                    int written = snprintf(_i2cAddresses + offset, 
                                          sizeof(_i2cAddresses) - offset, 
                                          "0x%02x", address);
                    if (written > 0) offset += written;
                }
                
                nDevices++;
            }
            
            yield();  // Allow ESP8266 background tasks
        }
        
        if (nDevices == 0) {
            DEBUG_PRINTLN(F("[SensorManager] No I2C devices found"));
        } else {
            DEBUG_PRINTF("[SensorManager] Found %u devices: %s\n", 
                          nDevices, _i2cAddresses);
        }
    }
    
    void initializeSensors() {
        // BMX280 at 0x76 or 0x77
        if (hasI2CAddress(I2CAddress::BMX280_SECONDARY)) {
            _bmx280 = new BMX280Sensor(I2CAddress::BMX280_SECONDARY);
            if (!_bmx280->begin()) {
                DEBUG_PRINTLN(F("[SensorManager] BMX280 init failed at 0x77"));
            }
        } else if (hasI2CAddress(I2CAddress::BMX280_PRIMARY)) {
            _bmx280 = new BMX280Sensor(I2CAddress::BMX280_PRIMARY);
            if (!_bmx280->begin()) {
                DEBUG_PRINTLN(F("[SensorManager] BMX280 init failed at 0x76"));
            }
        }
        
        // AHT20 at 0x38
        if (hasI2CAddress(I2CAddress::AHT20)) {
            _aht20 = new AHT20Sensor();
            if (_aht20->begin()) {
                _aht20->setAvailable(true);
            } else {
                DEBUG_PRINTLN(F("[SensorManager] AHT20 init failed"));
            }
        }
        
        // SCD4x at 0x62
        if (hasI2CAddress(I2CAddress::SCD4X)) {
            _scd4x = new SCD4xSensor();
            if (!_scd4x->begin()) {
                DEBUG_PRINTLN(F("[SensorManager] SCD4x init failed"));
            }
        }
        
        // SPS30 at 0x69
        if (hasI2CAddress(I2CAddress::SPS30)) {
            _sps30 = new SPS30Sensor();
            if (!_sps30->begin()) {
                DEBUG_PRINTLN(F("[SensorManager] SPS30 init failed"));
            }
        }
    }
    
    void logSensorStatus() {
        DEBUG_PRINTF("[SensorManager] Active sensors: %u\n", getSensorCount());
        if (_bmx280 && _bmx280->isAvailable()) 
            DEBUG_PRINTLN(F("[SensorManager] - BMX280: OK"));
        if (_aht20 && _aht20->isAvailable()) 
            DEBUG_PRINTLN(F("[SensorManager] - AHT20: OK"));
        if (_scd4x && _scd4x->isAvailable()) 
            DEBUG_PRINTLN(F("[SensorManager] - SCD4x: OK"));
        if (_sps30 && _sps30->isAvailable()) 
            DEBUG_PRINTLN(F("[SensorManager] - SPS30: OK"));
    }
    
};
