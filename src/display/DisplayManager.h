#pragma once

#include "../DebugConfig.h"
#include <Arduino.h>
#include <GxEPD.h>

// Panel selection: the TeHyBug CO2 Desk ships with either a 1.50" 200x200
// panel (default) or a 1.54" 152x152 panel. Build the small-panel variant
// with -DDISPLAY_152X152=1 (./build.sh handles this).
#ifndef DISPLAY_152X152
#define DISPLAY_152X152 0
#endif
#if DISPLAY_152X152
#include <GxDEPG0154BxS800FxX_BW/GxDEPG0154BxS800FxX_BW.h>  // 1.54" b/w 152x152
#else
#include <GxDEPG0150BN2/GxDEPG0150BN2.h>  // 1.50" b/w 200x200
#endif

#include <GxIO/GxIO.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <Adafruit_GFX.h>
#include <ArduinoJson.h>

// Fonts
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#if DISPLAY_152X152
#include <Fonts/FreeSans7pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#endif

// Forward declaration for logo
extern const unsigned char tehybug_logo_white[];
extern const unsigned char wifi_icon[];
#if DISPLAY_152X152
extern const unsigned char wifi_icon_small[];
#endif

/**
 * @brief Display configuration constants
 */
namespace DisplayConfig {
    // SPI pins
    const int8_t PIN_CS = 16;
    const int8_t PIN_DC = 15;
    const int8_t PIN_RST = -1;   // No reset pin
    const int8_t PIN_BUSY = -1;  // No busy pin

    const uint8_t ROTATION = 1;

    // Update settings
    const uint8_t FULL_REFRESH_INTERVAL = 30;  // Full refresh every N partial updates
    const uint16_t MIN_UPDATE_INTERVAL = 1000; // Minimum ms between updates

    // Logo settings
    const uint16_t LOGO_WIDTH = 180;
    const uint16_t LOGO_HEIGHT = 180;

    // Icon settings
#if DISPLAY_152X152
    const uint8_t WIFI_ICON_SIZE = 16;
#else
    const uint8_t WIFI_ICON_SIZE = 24;
#endif
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

        // Center the logo; on the 152x152 panel the 180x180 logo is larger
        // than the screen, so pin it to the origin (GxEPD clips the rest).
        uint16_t x = _display.width() > DisplayConfig::LOGO_WIDTH
                         ? (_display.width() - DisplayConfig::LOGO_WIDTH) / 2 : 0;
        uint16_t y = _display.height() > DisplayConfig::LOGO_HEIGHT
                         ? (_display.height() - DisplayConfig::LOGO_HEIGHT) / 2 : 0;

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
     * @brief Update display with sensor data
     * @param data JSON document with sensor values
     * @param imperialTemp Use Fahrenheit instead of Celsius
     * @param offlineMode Hide WiFi icon in offline mode
     */
    void updateSensorDisplay(const DynamicJsonDocument& data, bool imperialTemp, bool offlineMode) {
        if (!isReady()) return;

        DEBUG_PRINTLN(F("[Display] Received data:"));
        DEBUG_PRINT_JSON(data);
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
        
#if DISPLAY_152X152
        // The small panel only has room for the CO2 reading in the middle
        // (matching the original firmware's 152x152 layout, which had no
        // PM2.5 section).
        if (data.containsKey("co2")) {
            DEBUG_PRINTF("[Display] Drawing CO2: %s\n", data["co2"].as<String>().c_str());
            drawCO2Only(data);
        } else {
            DEBUG_PRINTLN(F("[Display] No CO2 data"));
        }
#else
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
#endif
        
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
    
    bool isReady() const { return _initialized; }

private:
    GxIO_Class _io;
    GxEPD_Class _display;

    bool _initialized = false;
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

#if DISPLAY_152X152
        _display.setFont(&FreeSans18pt7b);
        _display.setCursor(0, 30);
        _display.getTextBounds(intPart, 0, 0, &tbx, &tby, &tbw, &tbh);
        _display.print(intPart);

        _display.setFont(&FreeSans7pt7b);
        _display.setCursor(tbw - 4, 11);
        _display.print("o");  // Degree symbol
        _display.setCursor(tbw + 6, 14);
        _display.print(unit);
        _display.setCursor(tbw + 6, 30);
        _display.print(decPart);
#else
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
#endif
    }

    void drawHumidity(const DynamicJsonDocument& data) {
        const char* humiKey = "humi";
        if (!data.containsKey(humiKey)) return;
        String humiStr = String(data[humiKey].as<float>(), 0);

#if DISPLAY_152X152
        _display.setFont(&FreeSans18pt7b);
        _display.setCursor(90, 30);
        _display.print(humiStr.c_str());

        _display.setFont(&FreeSans7pt7b);
        _display.setCursor(130, 14);
        _display.print("%");
        _display.setCursor(130, 30);
        _display.print("RH");
#else
        _display.setFont(&FreeSans24pt7b);
        _display.setCursor(120, 44);
        _display.print(humiStr.c_str());

        _display.setFont(&FreeSans9pt7b);
        _display.setCursor(174, 24);
        _display.print("%");
        _display.setCursor(174, 44);
        _display.print("RH");
#endif
    }

#if !DISPLAY_152X152
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
#endif

    void drawCO2Only(const DynamicJsonDocument& data) {
        const char* co2Key = "co2";
        if (!data.containsKey(co2Key)) return;

        int16_t tbx, tby;
        uint16_t tbw, tbh;

        String co2Str = String(data[co2Key].as<float>(), 0);

#if DISPLAY_152X152
        _display.setTextSize(1);
        _display.setFont(&FreeSansBold24pt7b);
        _display.getTextBounds(co2Str.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t x = ((_display.width() - tbw) / 2) - tbx;
        _display.setCursor(x, 86);
        _display.print(co2Str.c_str());

        _display.setFont(&FreeSans9pt7b);
        _display.setCursor(70, 106);
        _display.print("CO2 PPM");
#else
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
#endif
    }

#if !DISPLAY_152X152
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
#endif

    void drawPressure(const DynamicJsonDocument& data) {
        const char* qfeKey = "qfe";
        if (!data.containsKey(qfeKey)) return;
        int16_t tbx, tby;
        uint16_t tbw, tbh;

        String qfeStr = String(data[qfeKey].as<float>(), 0);

#if DISPLAY_152X152
        const uint16_t baseline = 150;
#else
        const uint16_t baseline = 197;
#endif
        _display.setFont(&FreeSans18pt7b);
        _display.setCursor(0, baseline);
        _display.print(qfeStr.c_str());

        _display.getTextBounds(qfeStr.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
        _display.setFont(&FreeSans9pt7b);
        _display.setCursor(tbw + 6, baseline);
        _display.print("hPa");
    }

    void drawWiFiIcon() {
        uint16_t x = _display.width() - DisplayConfig::WIFI_ICON_SIZE;
        uint16_t y = _display.height() - DisplayConfig::WIFI_ICON_SIZE;
#if DISPLAY_152X152
        _display.drawExampleBitmap(wifi_icon_small, x, y,
                                   DisplayConfig::WIFI_ICON_SIZE, DisplayConfig::WIFI_ICON_SIZE,
                                   GxEPD_BLACK, GxEPD::bm_invert);
#else
        _display.drawExampleBitmap(wifi_icon, x, y,
                                   DisplayConfig::WIFI_ICON_SIZE, DisplayConfig::WIFI_ICON_SIZE,
                                   GxEPD_BLACK, GxEPD::bm_invert);
#endif
    }
};
