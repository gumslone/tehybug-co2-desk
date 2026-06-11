#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <functional>
#include "../display/DisplayManager.h"

/**
 * @brief Calculate CRC according to Sensirion datasheet
 */
inline uint8_t calcCrc(uint8_t data[2]) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < 2; i++) {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31u;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

/**
 * @brief SCD4x sensor calibration routines
 */
class SensorCalibration {
public:
    /**
     * @brief Calibrate SCD4x CO2 sensor to 400ppm (outdoor reference)
     * @param display Display manager for status messages
     * @param onStart Callback when calibration starts (e.g., disable sensor reading)
     * @param onEnd Callback when calibration ends (e.g., enable sensor reading)
     */
    static void calibrateSCD4x(DisplayManager& display,
                               std::function<void()> onStart = nullptr,
                               std::function<void()> onEnd = nullptr) {
        if (onStart) onStart();
        
        display.showMessage("Calibration started", "Put sensor", "outside and",
                           "wait for 15 minutes");
        
        DEBUG_PRINTLN(F("[Calibration] SCD4x calibration started"));
        DEBUG_PRINTLN(F("[Calibration] Place sensor outdoors for accurate results"));
        
        // Initialize I2C
        Wire.begin(0, 2);
        delay(1000);  // Wait for sensor ready
        
        // Start periodic measurement
        startPeriodicMeasurement();
        delay(2000);
        
        DEBUG_PRINTLN(F("[Calibration] Reading initial CO2 values..."));
        
        // Take 5 initial readings
        for (uint8_t i = 0; i < 5; i++) {
            float co2, temp, humidity;
            readMeasurement(co2, temp, humidity);
            DEBUG_PRINTF("[Calibration] Reading %d: CO2=%.0f ppm, T=%.1f°C, RH=%.1f%%\n",
                         i + 1, co2, temp, humidity);
            delay(2000);
        }
        
        // Wait 5 minutes for equilibration
        DEBUG_PRINTLN(F("[Calibration] Waiting 5 minutes for sensor equilibration..."));
        display.showMessage("Calibrating...", "Please wait", "5 minutes");
        delay(5UL * 60UL * 1000UL);
        
        // Stop measurement for calibration
        stopMeasurement();
        delay(20);
        
        // Perform forced recalibration to 400 ppm (outdoor reference)
        uint16_t targetCO2 = 400;
        int16_t correction = performForcedRecalibration(targetCO2);
        
        DEBUG_PRINTF("[Calibration] Calibration correction: %d\n", correction);
        
        // Restart periodic measurement
        startPeriodicMeasurement();
        
        char correctionStr[32];
        snprintf(correctionStr, sizeof(correctionStr), "Correction: %d", correction);
        display.showMessage("Calibration", "finished!", "", correctionStr);
        
        DEBUG_PRINTLN(F("[Calibration] Calibration complete"));
        
        delay(10000);
        
        if (onEnd) onEnd();
    }

private:
    static const uint8_t SCD4X_ADDR = 0x62;
    
    static void startPeriodicMeasurement() {
        Wire.beginTransmission(SCD4X_ADDR);
        Wire.write(0x21);
        Wire.write(0xb1);
        Wire.endTransmission();
    }
    
    static void stopMeasurement() {
        Wire.beginTransmission(SCD4X_ADDR);
        Wire.write(0x3f);
        Wire.write(0x86);
        Wire.endTransmission();
    }
    
    static void readMeasurement(float& co2, float& temperature, float& humidity) {
        uint8_t data[12];
        
        Wire.requestFrom(SCD4X_ADDR, (uint8_t)12);
        uint8_t counter = 0;
        while (Wire.available() && counter < 12) {
            data[counter++] = Wire.read();
        }
        
        co2 = (float)((uint16_t)data[0] << 8 | data[1]);
        temperature = -45 + 175 * (float)((uint16_t)data[3] << 8 | data[4]) / 65536;
        humidity = 100 * (float)((uint16_t)data[6] << 8 | data[7]) / 65536;
    }
    
    static int16_t performForcedRecalibration(uint16_t targetCO2) {
        uint8_t data[3];
        
        // Prepare calibration data with CRC
        data[0] = (targetCO2 & 0xff00) >> 8;
        data[1] = targetCO2 & 0x00ff;
        data[2] = calcCrc(data);
        
        // Send forced recalibration command
        Wire.beginTransmission(SCD4X_ADDR);
        Wire.write(0x36);
        Wire.write(0x2F);
        Wire.write(data[0]);
        Wire.write(data[1]);
        Wire.write(data[2]);
        Wire.endTransmission();
        
        delay(400);
        
        // Read correction value
        Wire.requestFrom(SCD4X_ADDR, (uint8_t)3);
        uint8_t counter = 0;
        while (Wire.available() && counter < 3) {
            data[counter++] = Wire.read();
        }
        
        if (calcCrc(data) != data[2]) {
            DEBUG_PRINTLN(F("[Calibration] CRC error in calibration response"));
            return 0;
        }
        
        uint16_t rawCorrection = ((uint16_t)data[0] << 8 | data[1]);
        return rawCorrection - 32768;
    }
};
