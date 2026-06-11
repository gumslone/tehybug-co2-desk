#pragma once

#include "../DebugConfig.h"
#include <Arduino.h>
#include <GxEPD.h>
#include <GxDEPG0150BN2/GxDEPG0150BN2.h>
#include <GxIO/GxIO.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <Adafruit_GFX.h>
#include <ArduinoJson.h>

// Fonts
#include <Fonts/FreeSans7pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// Forward declaration for logo
extern const unsigned char tehybug_logo_white[];
extern const unsigned char wifi_icon[];
extern const unsigned char wifi_icon_small[];

/**
 * @brief Display configuration constants
 */
namespace DisplayConfig {
    // SPI pins
    const int8_t PIN_CS = 16;
    const int8_t PIN_DC = 15;
    const int8_t PIN_RST = -1;   // No reset pin
    const int8_t PIN_BUSY = -1;  // No busy pin
    
    // Display properties
    const uint16_t WIDTH = 200;
    const uint16_t HEIGHT = 200;
    const uint8_t ROTATION = 1;
    
    // Update settings
    const uint8_t FULL_REFRESH_INTERVAL = 30;  // Full refresh every N partial updates
    const uint16_t MIN_UPDATE_INTERVAL = 1000; // Minimum ms between updates
    
    // Logo settings
    const uint16_t LOGO_WIDTH = 180;
    const uint16_t LOGO_HEIGHT = 180;
    
    // Icon settings
    const uint8_t WIFI_ICON_SIZE = 24;
}

/**
 * @brief E-Paper display manager for TeHyBug
 * 
 * Handles initialization, updates, and rendering of sensor data
 * on a 1.54" e-paper display.
 */
class DisplayManager {
public:
    DisplayManager() 
        : _io(SPI, DisplayConfig::PIN_CS, DisplayConfig::PIN_DC, DisplayConfig::PIN_RST),
          _display(_io, DisplayConfig::PIN_RST, DisplayConfig::PIN_BUSY) {}
    
    // Prevent copying
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;
    
    /**
     * @brief Initialize the display
     * @return true if initialization successful
     */
    bool begin() {
        if (!_enabled) return false;
        
        _display.init();
        _display.setRotation(DisplayConfig::ROTATION);
        _display.fillScreen(GxEPD_WHITE);
        _display.setTextSize(1);
        _display.setTextColor(GxEPD_BLACK);
        
        _initialized = true;
        DEBUG_PRINTLN(F("[Display] Initialized"));
        return true;
    }
    
    /**
     * @brief Show the logo on startup
     */
    void showLogo() {
        if (!isReady()) return;
        
        _display.fillScreen(GxEPD_WHITE);
        
        // Center the logo
        uint16_t x = (_display.width() - DisplayConfig::LOGO_WIDTH) / 2;
        uint16_t y = (_display.height() - DisplayConfig::LOGO_HEIGHT) / 2;
        
        _display.drawExampleBitmap(tehybug_logo_white, x, y, 
                                   DisplayConfig::LOGO_WIDTH, DisplayConfig::LOGO_HEIGHT, 
                                   GxEPD_BLACK, GxEPD::bm_invert);
        _display.update();
        delay(500);
    }
    
    /**
     * @brief Show a multi-line message
     * @param line1-line6 Text lines to display
     * @param partial Use partial update (faster, less flicker)
     */
    void showMessage(const String& line1, const String& line2 = "", 
                     const String& line3 = "", const String& line4 = "",
                     const String& line5 = "", const String& line6 = "",
                     bool partial = true) {
        if (!isReady()) return;
        
        _display.fillScreen(GxEPD_WHITE);
        _display.setFont(&FreeSans12pt7b);
        _display.setCursor(0, 25);  // Start with proper baseline
        
        _display.println(line1);
        if (line2.length() > 0) _display.println(line2);
        if (line3.length() > 0) _display.println(line3);
        if (line4.length() > 0) _display.println(line4);
        if (line5.length() > 0) _display.println(line5);
        if (line6.length() > 0) _display.println(line6);
        
        if (partial) {
            _display.updateWindow(0, 0, _display.width(), _display.height());
        } else {
            _display.update();
        }
    }
    
    /**
     * @brief Show centered text message
     * @param text Text to display centered
     * @param font Font to use
     */
    void showCenteredText(const char* text, const GFXfont* font = &FreeSans18pt7b) {
        if (!isReady()) return;
        
        _display.fillScreen(GxEPD_WHITE);
        _display.setFont(font);
        
        int16_t x1, y1;
        uint16_t w, h;
        _display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
        
        uint16_t x = (_display.width() - w) / 2 - x1;
        uint16_t y = (_display.height() - h) / 2 - y1;
        
        _display.setCursor(x, y);
        _display.print(text);
        _display.updateWindow(0, 0, _display.width(), _display.height());
    }
    
    /**
     * @brief Update display with sensor data
     * @param data JSON document with sensor values
     * @param imperialTemp Use Fahrenheit instead of Celsius
     * @param offlineMode Hide WiFi icon in offline mode
     */
    void updateSensorDisplay(const DynamicJsonDocument& data, bool imperialTemp, bool offlineMode) {
        if (!isReady() || !_updateEnabled) return;
        
        // Debug: Print what we received
        DEBUG_PRINTLN(F("[Display] Received data:"));
        serializeJsonPretty(data, Serial);
        DEBUG_PRINTLN();
        
        // Rate limiting
        uint32_t now = millis();
        if (now - _lastUpdateTime < DisplayConfig::MIN_UPDATE_INTERVAL) {
            DEBUG_PRINTLN(F("[Display] Skipping update - too soon"));
            return;
        }
        _lastUpdateTime = now;
        
        // Full refresh periodically to prevent ghosting
        if (_updateCounter >= DisplayConfig::FULL_REFRESH_INTERVAL) {
            performFullRefresh();
        }
        
        _display.fillScreen(GxEPD_WHITE);
        _display.setTextSize(1);
        _display.setTextColor(GxEPD_BLACK);
        
        // Temperature (top left)
        if (data.containsKey("temp")) {
            DEBUG_PRINTF("[Display] Drawing temp: %s\n", data["temp"].as<String>().c_str());
            drawTemperature(data, imperialTemp);
        } else {
            DEBUG_PRINTLN(F("[Display] No temp data"));
        }
        
        // Humidity (top right)
        if (data.containsKey("humi")) {
            DEBUG_PRINTF("[Display] Drawing humidity: %s\n", data["humi"].as<String>().c_str());
            drawHumidity(data);
        } else {
            DEBUG_PRINTLN(F("[Display] No humidity data"));
        }
        
        // CO2 and PM2.5 (middle)
        if (data.containsKey("co2") && data.containsKey("pm25")) {
            DEBUG_PRINTF("[Display] Drawing CO2: %s, PM2.5: %s\n", 
                        data["co2"].as<String>().c_str(), data["pm25"].as<String>().c_str());
            drawCO2AndPM25(data);
        } else if (data.containsKey("co2")) {
            DEBUG_PRINTF("[Display] Drawing CO2 only: %s\n", data["co2"].as<String>().c_str());
            drawCO2Only(data);
        } else if (data.containsKey("pm25")) {
            DEBUG_PRINTF("[Display] Drawing PM2.5 only: %s\n", data["pm25"].as<String>().c_str());
            drawPM25Only(data);
        } else {
            DEBUG_PRINTLN(F("[Display] No CO2 or PM2.5 data"));
        }
        
        // Pressure (bottom)
        if (data.containsKey("qfe")) {
            DEBUG_PRINTF("[Display] Drawing pressure: %s\n", data["qfe"].as<String>().c_str());
            drawPressure(data);
        } else {
            DEBUG_PRINTLN(F("[Display] No pressure data"));
        }
        
        // WiFi icon (bottom right)
        if (!offlineMode) {
            drawWiFiIcon();
        }
        
        _display.updateWindow(0, 0, _display.width(), _display.height());
        _updateCounter++;
        DEBUG_PRINTLN(F("[Display] Update complete"));
    }
    
    /**
     * @brief Force a full display refresh
     */
    void forceFullRefresh() {
        if (!isReady()) return;
        performFullRefresh();
    }
    
    /**
     * @brief Clear the display
     */
    void clear() {
        if (!isReady()) return;
        _display.fillScreen(GxEPD_WHITE);
        _display.update();
    }
    
    // Getters and setters
    void setEnabled(bool enabled) { _enabled = enabled; }
    bool isEnabled() const { return _enabled; }
    
    void setUpdateEnabled(bool enabled) { _updateEnabled = enabled; }
    bool isUpdateEnabled() const { return _updateEnabled; }
    
    bool isInitialized() const { return _initialized; }
    bool isReady() const { return _enabled && _initialized; }
    
    uint16_t getWidth() const { return _display.width(); }
    uint16_t getHeight() const { return _display.height(); }
    
    GxEPD_Class& getDisplay() { return _display; }
    const GxEPD_Class& getDisplay() const { return _display; }

private:
    GxIO_Class _io;
    GxEPD_Class _display;
    
    bool _enabled = true;
    bool _initialized = false;
    bool _updateEnabled = true;
    uint8_t _updateCounter = 100;  // Start high to trigger initial full refresh
    uint32_t _lastUpdateTime = 0;
    
    /**
     * @brief Perform a full display refresh to clear ghosting
     */
    void performFullRefresh() {
        _updateCounter = 0;
        _display.setRotation(DisplayConfig::ROTATION);
        _display.fillScreen(GxEPD_WHITE);
        _display.setTextSize(1);
        _display.setTextColor(GxEPD_BLACK);
        _display.update();
        delay(500);
    }
    
    /**
     * @brief Helper to format and display a float value
     */
    void formatFloat(float value, char* buffer, size_t bufSize, uint8_t decimals = 1) {
        dtostrf(value, 1, decimals, buffer);
    }

    void drawTemperature(const DynamicJsonDocument& data, bool imperial) {
        const char* tempKey = imperial ? "temp_imp" : "temp";
        const char* unit = imperial ? "F" : "C";
        
        if (!data.containsKey(tempKey)) return;
        String tempString = String(data[tempKey].as<float>(), 1);
        // Split at decimal point
        char* dot = strchr(tempString.c_str(), '.');
        char intPart[8] = "";
        char decPart[4] = "";
        
        if (dot) {
            size_t intLen = dot - tempString.c_str();
            strncpy(intPart, tempString.c_str(), intLen);
            intPart[intLen] = '.';
            intPart[intLen + 1] = '\0';
            strncpy(decPart, dot + 1, 1);
            decPart[1] = '\0';
        } else {
            strncpy(intPart, tempString.c_str(), sizeof(intPart) - 1);
        }
        
        int16_t tbx, tby;
        uint16_t tbw, tbh;
        
        _display.setFont(&FreeSans24pt7b);
        _display.setCursor(0, 44);
        _display.getTextBounds(intPart, 0, 0, &tbx, &tby, &tbw, &tbh);
        _display.print(intPart);
        
        _display.setFont(&FreeSans9pt7b);
        _display.setCursor(tbw - 5, 21);
        _display.print("o");  // Degree symbol
        _display.setCursor(tbw + 6, 24);
        _display.print(unit);
        _display.setCursor(tbw + 6, 44);
        _display.print(decPart);
    }

    void drawHumidity(const DynamicJsonDocument& data) {
        const char* humiKey = "humi";
        if (!data.containsKey(humiKey)) return;
        String humiStr = String(data[humiKey].as<float>(), 0);
        
        _display.setFont(&FreeSans24pt7b);
        _display.setCursor(120, 44);
        _display.print(humiStr.c_str());
        
        _display.setFont(&FreeSans9pt7b);
        _display.setCursor(174, 24);
        _display.print("%");
        _display.setCursor(174, 44);
        _display.print("RH");
    }

    void drawCO2AndPM25(const DynamicJsonDocument& data) {        
        const char* pm25Key = "pm25";
        const char* c02Key = "co2";
        if(!data.containsKey(pm25Key) || !data.containsKey(c02Key)) return;
        int16_t tbx, tby;
        uint16_t tbw, tbh;
        String pm25Str = String(data[pm25Key].as<float>(), 0);
        
        _display.setFont(&FreeSans24pt7b);
        _display.getTextBounds(pm25Str.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
        _display.setCursor(0, 120);
        _display.print(pm25Str.c_str());
        
        _display.setFont(&FreeSans9pt7b);
        _display.setCursor(0, 146);
        _display.print("PM2.5");
        
        // CO2 (right side)
        String co2Str = String(data[c02Key].as<float>(), 0);
        
        _display.setFont(&FreeSans24pt7b);
        _display.getTextBounds(co2Str, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t x = _display.width() - 6 - tbw;
        _display.setCursor(x, 120);
        _display.print(co2Str);
        
        _display.setFont(&FreeSans9pt7b);
        _display.setCursor(120, 146);
        _display.print("CO2 PPM");
    }

    void drawCO2Only(const DynamicJsonDocument& data) {
        const char* co2Key = "co2";
        if (!data.containsKey(co2Key)) return;
        
        int16_t tbx, tby;
        uint16_t tbw, tbh;

        String co2Str = String(data[co2Key].as<float>(), 0);
        
        _display.setTextSize(2);
        _display.setFont(&FreeSansBold18pt7b);
        _display.getTextBounds(co2Str.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t x = ((_display.width() - tbw) / 2) - tbx;
        _display.setCursor(x, 120);
        _display.print(co2Str.c_str());
        
        _display.setTextSize(1);
        _display.setFont(&FreeSans12pt7b);
        _display.setCursor(92, 146);
        _display.print("CO2 PPM");
    }

    void drawPM25Only(const DynamicJsonDocument& data) {
        const char* pm25Key = "pm25";
        if (!data.containsKey(pm25Key)) return;

        int16_t tbx, tby;
        uint16_t tbw, tbh;

        String pm25Str = String(data[pm25Key].as<float>(), 0);
        
        _display.setTextSize(2);
        _display.setFont(&FreeSansBold18pt7b);
        _display.getTextBounds(pm25Str, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t x = ((_display.width() - tbw) / 2) - tbx;
        _display.setCursor(x, 120);
        _display.print(pm25Str.c_str());
        
        _display.setTextSize(1);
        _display.setFont(&FreeSans12pt7b);
        _display.setCursor(80, 146);
        _display.print("PM2.5");
    }

    void drawPressure(const DynamicJsonDocument& data) {
        const char* qfeKey = "qfe";
        if (!data.containsKey(qfeKey)) return;
        int16_t tbx, tby;
        uint16_t tbw, tbh;

        String qfeStr = String(data[qfeKey].as<float>(), 0);
        
        _display.setFont(&FreeSans18pt7b);
        _display.setCursor(0, 197);
        _display.print(qfeStr.c_str());
        
        _display.getTextBounds(qfeStr.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
        _display.setFont(&FreeSans9pt7b);
        _display.setCursor(tbw + 6, 197);
        _display.print("hPa");
    }
    
    void drawWiFiIcon() {
        uint16_t x = _display.width() - DisplayConfig::WIFI_ICON_SIZE;
        uint16_t y = _display.height() - DisplayConfig::WIFI_ICON_SIZE;
        _display.drawExampleBitmap(wifi_icon, x, y, 
                                   DisplayConfig::WIFI_ICON_SIZE, DisplayConfig::WIFI_ICON_SIZE, 
                                   GxEPD_BLACK, GxEPD::bm_invert);
    }
};
