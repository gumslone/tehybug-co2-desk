#pragma once

#include <Arduino.h>
#include "ISensor.h"
#include <AHT20.h>

/**
 * @brief AHT20 Temperature and Humidity sensor implementation
 */
class AHT20Sensor : public ISensor {
public:
    bool begin() override {
        // AHT20 doesn't have a begin method that returns status
        // We assume it's available if I2C scanner found it
        _available = true;
        DEBUG_PRINTLN(F("[AHT20] Initialized"));
        return true;
    }
    
    bool isAvailable() const override { return _available; }
    
    void read(JsonDocument& data) override {
        if (!_available) return;
        
        float humidity, temperature;
        int ret = _sensor.getSensor(&humidity, &temperature);
        
        if (ret) {
            data["temp"] = temperature;
            data["humi"] = humidity * 100;
            
            DEBUG_PRINTF("[AHT20] T: %.1f°C, H: %.1f%%\n", 
                        temperature, humidity * 100);
        } else {
            DEBUG_PRINTLN(F("[AHT20] Failed to read data"));
        }
    }
    
    const char* getName() const override { return "AHT20"; }
    
    void setAvailable(bool available) { _available = available; }

private:
    AHT20 _sensor;
    bool _available = false;
};
