#pragma once

#include <Arduino.h>
#include "ISensor.h"
#include "SparkFun_SCD4x_Arduino_Library.h"

/**
 * @brief SCD4x CO2 sensor implementation
 */
class SCD4xSensor : public ISensor {
public:
    bool begin() override {
        if (!_sensor.begin()) {
            DEBUG_PRINTLN(F("[SCD4x] Sensor not detected"));
            _available = false;
            return false;
        }
        
        _available = true;
        _lastMeasurement = millis();
        DEBUG_PRINTLN(F("[SCD4x] Initialized successfully"));
        return true;
    }
    
    bool isAvailable() const override { return _available; }
    
    void read(JsonDocument& data) override {
        read(data, false, false);
    }
    
    /**
     * @brief Read sensor with options for secondary readings
     * @param data JSON document to store readings
     * @param hasOtherTempSensor If true, store temp/humi as temp2/humi2
     * @param hasOtherHumiSensor If true, store humi as humi2
     */
    void read(JsonDocument& data, bool hasOtherTempSensor, bool hasOtherHumiSensor) {
        if (!_available) return;
        
        // SCD4x updates every 5 seconds
        if (millis() - _lastMeasurement < 5000) {
            return;
        }
        
        if (_sensor.readMeasurement()) {
            float co2 = _sensor.getCO2();
            float temp = _sensor.getTemperature();
            float humi = _sensor.getHumidity();
            
            if (co2 > 0) {
                data["co2"] = co2;
                _lastCO2 = co2;
                
                // Store as secondary if other sensors provide primary temp/humi
                if (hasOtherTempSensor || hasOtherHumiSensor) {
                    data["temp2"] = temp;
                    data["humi2"] = humi;
                } else {
                    data["temp"] = temp;
                    data["humi"] = humi;
                }
                
                DEBUG_PRINTF("[SCD4x] CO2: %.0f ppm, T: %.1f°C, H: %.1f%%\n", 
                            co2, temp, humi);
            } else {
                DEBUG_PRINTLN(F("[SCD4x] CO2 reading is 0, skipping"));
            }
        }
        
        _lastMeasurement = millis();
    }
    
    const char* getName() const override { return "SCD4x"; }
    
    float getLastCO2() const { return _lastCO2; }
    
    SCD4x& getSensor() { return _sensor; }

private:
    SCD4x _sensor;
    bool _available = false;
    unsigned long _lastMeasurement = 0;
    float _lastCO2 = 0;
};
