#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "ISensor.h"
#include <SensirionI2cSps30.h>

// Ensure NO_ERROR is properly defined
#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

/**
 * @brief SPS30 Particulate Matter sensor implementation
 */
class SPS30Sensor : public ISensor {
public:
    bool begin() override {
        _sensor.begin(Wire, SPS30_I2C_ADDR_69);
        _sensor.stopMeasurement();
        
        int8_t serialNumber[32] = {0};
        int8_t productType[8] = {0};
        
        _sensor.readSerialNumber(serialNumber, 32);
        DEBUG_PRINT(F("[SPS30] Serial: "));
        DEBUG_PRINTLN((const char*)serialNumber);
        
        _sensor.readProductType(productType, 8);
        DEBUG_PRINT(F("[SPS30] Product: "));
        DEBUG_PRINTLN((const char*)productType);
        
        _sensor.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_UINT16);
        delay(100);
        
        _available = true;
        _enabled = true;
        DEBUG_PRINTLN(F("[SPS30] Initialized successfully"));
        return true;
    }
    
    bool isAvailable() const override { return _available; }
    
     /**
     * @brief Read sensor data and store in JSON document
     * @param data JSON document to store sensor readings
    */
    void read(JsonDocument& data) override {
        if (!_available || !_enabled) return;
        
        uint16_t dataReadyFlag = 0;
        uint16_t mc1p0, mc2p5, mc4p0, mc10p0;
        uint16_t nc0p5, nc1p0, nc2p5, nc4p0, nc10p0;
        uint16_t typicalParticleSize;
        
        delay(1000);
        
        int16_t error = _sensor.readDataReadyFlag(dataReadyFlag);
        if (error != NO_ERROR) {
            char errorMsg[64];
            errorToString(error, errorMsg, sizeof(errorMsg));
            DEBUG_PRINTF("[SPS30] readDataReadyFlag error: %s\n", errorMsg);
            return;
        }
        
        error = _sensor.readMeasurementValuesUint16(
            mc1p0, mc2p5, mc4p0, mc10p0,
            nc0p5, nc1p0, nc2p5, nc4p0,
            nc10p0, typicalParticleSize
        );
        
        if (error != NO_ERROR) {
            char errorMsg[64];
            errorToString(error, errorMsg, sizeof(errorMsg));
            DEBUG_PRINTF("[SPS30] readMeasurement error: %s\n", errorMsg);
            return;
        }
        
        char valueStr[16];
        snprintf(valueStr, sizeof(valueStr), "%u", mc2p5);
        data["pm25"] = valueStr;
        
        DEBUG_PRINTF("[SPS30] PM1.0: %u, PM2.5: %u, PM4.0: %u, PM10: %u\n",
                      mc1p0, mc2p5, mc4p0, mc10p0);
    }
    
    const char* getName() const override { return "SPS30"; }
    
    void setAvailable(bool available) { _available = available; }
    
    /**
     * @brief Enable or disable the SPS30 sensor
     * @param enabled True to enable, false to disable
     */
    void setEnabled(bool enabled) {
        if (_enabled == enabled) return;
        
        _enabled = enabled;
        
        if (_available) {
            if (_enabled) {
                _sensor.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_UINT16);
                DEBUG_PRINTLN(F("[SPS30] Sensor enabled"));
            } else {
                _sensor.stopMeasurement();
                DEBUG_PRINTLN(F("[SPS30] Sensor disabled"));
            }
        }
    }
    
    /**
     * @brief Check if sensor is enabled
     * @return True if enabled, false otherwise
     */
    bool isEnabled() const { return _enabled; }

private:
    SensirionI2cSps30 _sensor;
    bool _available = false;
    bool _enabled = true;
};