#pragma once

#include "../DebugConfig.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

/**
 * @brief CO2 level thresholds for LED color indication (LED 1)
 */
namespace CO2Levels {
    const uint16_t GOOD_MAX = 1000;      // Green below this
    const uint16_t WARNING_MAX = 1500;   // Yellow below this, Red above
}

/**
 * @brief PM2.5 level thresholds for LED color indication (LED 0)
 * Based on EPA AQI breakpoints for PM2.5 (µg/m³)
 */
namespace PM25Levels {
    const uint16_t GOOD_MAX = 12;        // Green: Good (0-12)
    const uint16_t MODERATE_MAX = 35;    // Yellow: Moderate (12.1-35.4)
    // Above 35 = Red: Unhealthy for sensitive groups
}

/**
 * @brief LED colors as packed RGB values
 */
namespace LEDColors {
    const uint32_t RED = 0xFF0000;
    const uint32_t YELLOW = 0xFFC800;
    const uint32_t GREEN = 0x00FF00;
    const uint32_t BLUE = 0x0000FF;
    const uint32_t PINK = 0xFF00FF;
    const uint32_t OFF = 0x000000;
}

/**
 * @brief LED index definitions
 */
namespace LEDIndex {
    const uint8_t PM25 = 0;    // First LED for PM2.5
    const uint8_t CO2 = 1;     // Second LED for CO2
}

/**
 * @brief NeoPixel LED controller for air quality indication
 * LED 0: PM2.5 indicator
 * LED 1: CO2 indicator
 */
class LEDController {
public:
    LEDController(uint8_t pin, uint8_t count) 
        : _strip(count, pin, NEO_GRB + NEO_KHZ800), _pin(pin), _count(count) {}
    
    // Prevent copying
    LEDController(const LEDController&) = delete;
    LEDController& operator=(const LEDController&) = delete;
    
    void begin() {
        _strip.begin();
        _strip.show();
        DEBUG_PRINTLN(F("[LED] Initialized"));
    }
    
    /**
     * @brief Set LED brightness (0-255)
     */
    void setBrightness(uint8_t brightness) {
        _brightness = brightness;
        _strip.setBrightness(_brightness);
    }
    
    /**
     * @brief Set a single LED color
     * @param index LED index (0 or 1)
     * @param color Packed RGB color
     */
    void setPixelColor(uint8_t index, uint32_t color) {
        if (index < _count) {
            _strip.setBrightness(_brightness);
            _strip.setPixelColor(index, color);
            _strip.show();
        }
    }
    
    /**
     * @brief Fill all LEDs with a color with wipe animation
     * @param color Packed RGB color
     * @param wait Delay between pixels in ms
     */
    void colorWipe(uint32_t color, uint8_t wait = 90) {
        _strip.setBrightness(_brightness);
        for (uint8_t i = 0; i < _strip.numPixels(); i++) {
            _strip.setPixelColor(i, color);
            _strip.show();
            delay(wait);
        }
    }
    
    /**
     * @brief Set all LEDs to a color immediately
     */
    void setColor(uint32_t color) {
        _strip.setBrightness(_brightness);
        for (uint8_t i = 0; i < _strip.numPixels(); i++) {
            _strip.setPixelColor(i, color);
        }
        _strip.show();
    }
    
    /**
     * @brief Update PM2.5 indicator on LED 0
     * @param pm25 PM2.5 value in µg/m³
     */
    void updatePM25Indicator(float pm25) {
        uint32_t color;
        if (pm25 > PM25Levels::MODERATE_MAX) {
            color = LEDColors::RED;      // Unhealthy
        } else if (pm25 > PM25Levels::GOOD_MAX) {
            color = LEDColors::YELLOW;   // Moderate
        } else {
            color = LEDColors::GREEN;    // Good
        }
        setPixelColor(LEDIndex::PM25, color);
    }
    
    /**
     * @brief Update CO2 indicator on LED 1
     * @param co2 CO2 value in ppm
     */
    void updateCO2Indicator(float co2) {
        uint32_t color;
        if (co2 > CO2Levels::WARNING_MAX) {
            color = LEDColors::RED;
        } else if (co2 > CO2Levels::GOOD_MAX) {
            color = LEDColors::YELLOW;
        } else {
            color = LEDColors::GREEN;
        }
        setPixelColor(LEDIndex::CO2, color);
    }
    
    /**
     * @brief Update both LEDs with CO2 indicator (when PM2.5 not available)
     * @param co2 CO2 value in ppm
     */
    void updateCO2IndicatorBothLEDs(float co2) {
        uint32_t color;
        if (co2 > CO2Levels::WARNING_MAX) {
            color = LEDColors::RED;
        } else if (co2 > CO2Levels::GOOD_MAX) {
            color = LEDColors::YELLOW;
        } else {
            color = LEDColors::GREEN;
        }
        _strip.setBrightness(_brightness);
        _strip.setPixelColor(LEDIndex::PM25, color);
        _strip.setPixelColor(LEDIndex::CO2, color);
        _strip.show();
    }
    
    /**
     * @brief Update both air quality indicators
     * @param pm25 PM2.5 value in µg/m³ (use negative to skip)
     * @param co2 CO2 value in ppm (use negative to skip)
     */
    void updateAirQualityIndicators(float pm25, float co2) {
        _strip.setBrightness(_brightness);
        
        // Update PM2.5 on LED 0
        if (pm25 >= 0) {
            uint32_t pm25Color;
            if (pm25 > PM25Levels::MODERATE_MAX) {
                pm25Color = LEDColors::RED;
            } else if (pm25 > PM25Levels::GOOD_MAX) {
                pm25Color = LEDColors::YELLOW;
            } else {
                pm25Color = LEDColors::GREEN;
            }
            _strip.setPixelColor(LEDIndex::PM25, pm25Color);
        }
        
        // Update CO2 on LED 1
        if (co2 >= 0) {
            uint32_t co2Color;
            if (co2 > CO2Levels::WARNING_MAX) {
                co2Color = LEDColors::RED;
            } else if (co2 > CO2Levels::GOOD_MAX) {
                co2Color = LEDColors::YELLOW;
            } else {
                co2Color = LEDColors::GREEN;
            }
            _strip.setPixelColor(LEDIndex::CO2, co2Color);
        }
        
        _strip.show();
    }
    
    /**
     * @brief Show initialization color
     */
    void showInit() {
        colorWipe(LEDColors::BLUE, 10);
    }
    
    /**
     * @brief Show WiFi toggle indication
     */
    void showWiFiToggle() {
        colorWipe(LEDColors::PINK, 10);
    }
    
    /**
     * @brief Turn off all LEDs
     */
    void off() {
        colorWipe(LEDColors::OFF, 10);
    }
    
    /**
     * @brief Get the underlying strip for advanced operations
     */
    Adafruit_NeoPixel& getStrip() { return _strip; }

private:
    Adafruit_NeoPixel _strip;
    uint8_t _pin;
    uint8_t _count;
    uint8_t _brightness = 200;
};
