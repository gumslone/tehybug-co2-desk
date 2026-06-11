#pragma once

#include <ArduinoJson.h>

/**
 * @brief Abstract interface for all sensors
 * 
 * All sensor implementations should inherit from this class
 * to ensure consistent behavior across different sensor types.
 */
class ISensor {
public:
    virtual ~ISensor() = default;
    
    /**
     * @brief Initialize the sensor
     * @return true if initialization successful, false otherwise
     */
    virtual bool begin() = 0;
    
    /**
     * @brief Check if sensor is available and working
     * @return true if sensor is available
     */
    virtual bool isAvailable() const = 0;
    
    /**
     * @brief Read sensor data and store in JSON document
     * @param data JSON document to store sensor readings
     */
    virtual void read(JsonDocument& data) = 0;
    
    /**
     * @brief Get the sensor name for logging/debugging
     * @return Sensor name string
     */
    virtual const char* getName() const = 0;
};
