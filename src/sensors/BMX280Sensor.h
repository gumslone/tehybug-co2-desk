#pragma once

#include <Arduino.h>
#include "ISensor.h"
#include <ErriezBMX280.h>

// Adjust sea level for altitude calculation
#ifndef SEA_LEVEL_PRESSURE_HPA
#define SEA_LEVEL_PRESSURE_HPA 1026.25
#endif

/**
 * @brief BMX280/BME280/BMP280 sensor implementation
 */
class BMX280Sensor : public ISensor {
public:
    BMX280Sensor(uint8_t address = 0x76) : _sensor(address) {}
    
    bool begin() override {
        if (!_sensor.begin()) {
            DEBUG_PRINTLN(F("[BMX280] Sensor not detected"));
            _available = false;
            return false;
        }
        
        // Print sensor type
        DEBUG_PRINT(F("[BMX280] Sensor type: "));
        switch (_sensor.getChipID()) {
            case CHIP_ID_BMP280:
                DEBUG_PRINTLN(F("BMP280"));
                _isBME = false;
                break;
            case CHIP_ID_BME280:
                DEBUG_PRINTLN(F("BME280"));
                _isBME = true;
                break;
            default:
                DEBUG_PRINTLN(F("Unknown"));
                break;
        }
        
        // Configure sampling for indoor navigation mode
        _sensor.setSampling(
            BMX280_MODE_NORMAL,
            BMX280_SAMPLING_X16,    // Temp
            BMX280_SAMPLING_X16,    // Press
            BMX280_SAMPLING_X16,    // Hum (BME280 only)
            BMX280_FILTER_X16,
            BMX280_STANDBY_MS_500
        );
        
        _available = true;
        DEBUG_PRINTLN(F("[BMX280] Initialized successfully"));
        return true;
    }
    
    bool isAvailable() const override { return _available; }
    
    void read(JsonDocument& data) override {
        if (!_available) return;
        
        float temp = _sensor.readTemperature();
        float pressure = _sensor.readPressure() / 100.0F;
        float altitude = _sensor.readAltitude(SEA_LEVEL_PRESSURE_HPA);
        
        data["temp"] = temp;
        data["qfe"] = pressure;
        data["alt"] = altitude;
        
        // BME280 also has humidity
        if (_isBME) {
            float humidity = _sensor.readHumidity();
            data["humi"] = humidity;
        }
        
        DEBUG_PRINTF("[BMX280] T: %.1f°C, P: %.0f hPa, Alt: %.1f m\n", 
                    temp, pressure, altitude);
    }
    
    const char* getName() const override { return "BMX280"; }
    
    bool isBME280() const { return _isBME; }

private:
    ErriezBMX280 _sensor;
    bool _available = false;
    bool _isBME = false;
};
