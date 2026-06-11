/**
 * TeHyBug CO2 Desk Firmware
 * 
 * A modular firmware for ESP8266-based CO2 monitoring device
 * with e-paper display, NeoPixel LEDs, and MQTT/Home Assistant integration.
 * 
 * Hardware:
 * - ESP8266 (NodeMCU/Wemos D1 Mini)
 * - SCD4x CO2 sensor
 * - BME280/BMP280 pressure/temperature sensor (optional)
 * - AHT20 temperature/humidity sensor (optional)
 * - SPS30 particulate matter sensor (optional)
 * - 1.5" E-Paper display
 * - WS2812 NeoPixel LEDs
 * 
 * @version 10.04.2025
 */

// ============================================================================
// INCLUDES
// ============================================================================
// Debug control MUST be included first
#include "src/DebugConfig.h"

// Core modules
#include "src/AppConfig.h"
#include "src/sensors/SensorManager.h"
#include "src/display/DisplayManager.h"
#include "src/led/LEDController.h"
#include "src/input/ButtonManager.h"
#include "src/network/NetworkManager.h"
#include "src/calibration/SensorCalibration.h"

// External libraries
#include <TickerScheduler.h>
#include <ESP8266WiFi.h>

// Assets
#include "images.h"
#include "Webinterface.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

namespace Pins {
    const uint8_t LED_STRIP = 12;
    const uint8_t I2C_SDA = 0;
    const uint8_t I2C_SCL = 2;
}

namespace Hardware {
    const uint8_t LED_COUNT = 2;
    const uint32_t SENSOR_READ_INTERVAL = 10033;  // ~10 seconds
}

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

const char* const VERSION = "09.03.2026";

// Hardware managers
SensorManager sensors;
DisplayManager display;
LEDController leds(Pins::LED_STRIP, Hardware::LED_COUNT);
ButtonManager buttons;
NetworkManager network;

// Scheduler
TickerScheduler ticker(5);

// Remote sensor flag
bool useRemoteSensor = false;

// ============================================================================
// CALLBACKS & HANDLERS
// ============================================================================

/**
 * @brief Read all sensors and update display
 */
void readSensorsTask() {
   sensors.readAll();
   
   // Debug: Print sensor data
   DEBUG_PRINTLN(F("\n[Main] Sensor readings:"));
   const auto& data = sensors.getData();
   serializeJsonPretty(data, Serial);
   DEBUG_PRINTLN();
   
   // Handle remote sensor if no local sensors
   if (useRemoteSensor && !AppConfig::instance().isOfflineMode()) {
       network.httpGet("http://tehybug.local", [&](JsonDocument& json) {
           if (json.containsKey("co2")) sensors.addData("co2", json["co2"].as<float>());
           if (json.containsKey("temp")) sensors.addData("temp", json["temp"].as<float>());
           if (json.containsKey("humi")) sensors.addData("humi", json["humi"].as<float>());
           if (json.containsKey("qfe")) sensors.addData("qfe", json["qfe"].as<float>());
           if (json.containsKey("alt")) sensors.addData("alt", json["alt"].as<float>());
           if (json.containsKey("pm25")) sensors.addData("pm25", json["pm25"].as<float>());
       });
   }
   
   // Update LED indicators
   float pm25 = sensors.getPM25();
   float co2 = sensors.getCO2();
   
   // Debug: Print extracted values
   DEBUG_PRINTF("[Main] CO2: %.1f, PM2.5: %.1f\n", co2, pm25);
   
   if (pm25 >= 0 && co2 >= 0) {
       leds.updateAirQualityIndicators(pm25, co2);
   } else if (pm25 >= 0) {
       leds.updatePM25Indicator(pm25);
   } else if (co2 >= 0) {
       leds.updateCO2IndicatorBothLEDs(co2);
   }
   
   // Update display
   display.updateSensorDisplay(sensors.getData(), AppConfig::instance().isImperialTemp(), AppConfig::instance().isOfflineMode());
}

/**
 * @brief Handle calibration request
 */
void onCalibrateSensor() {
    if (!sensors.hasSCD4x()) {
        DEBUG_PRINTLN(F("[Main] No SCD4x sensor for calibration"));
        return;
    }
    
    DEBUG_PRINTLN(F("[Main] Starting calibration..."));
    
    SensorCalibration::calibrateSCD4x(
        display,
        [&]() { ticker.disable(0); },  // Stop sensor reading during calibration
        [&]() { ticker.enable(0); }    // Resume sensor reading after calibration
    );
}

/**
 * @brief Handle WiFi reset request
 */
void onResetWiFi() {
    DEBUG_PRINTLN(F("[Main] Resetting WiFi settings..."));
    network.resetWiFiAndReboot();
}

/**
 * @brief Get main page JSON data
 */
String getMainPageData() {
    return network.getSensorJson(sensors.getData(), sensors.getI2CAddresses());
}

/**
 * @brief Save web configuration
 */
void saveWebConfig(bool imperialTemp, bool imperialQfe, uint8_t ledBrightness) {
    AppConfig::instance().setImperialTemp(imperialTemp);
    AppConfig::instance().setImperialQfe(imperialQfe);
    AppConfig::instance().setLedBrightness(ledBrightness);
    AppConfig::instance().save();
    leds.setBrightness(ledBrightness);
}

/**
 * @brief Get config page HTML
 */
String getConfigPage() {
    return configPage;
}

/**
 * @brief Toggle SPS30 sensor on/off
 */
void onToggleSPS30() {
    if (!sensors.hasSPS30()) {
        DEBUG_PRINTLN(F("[Main] No SPS30 sensor available"));
        return;
    }
    
    bool currentState = sensors.isSPS30Enabled();
    bool newState = !currentState;
    sensors.setSPS30Enabled(newState);
    
    // Save the state to config
    AppConfig::instance().setSPS30Enabled(newState);
    AppConfig::instance().save();
    
    String status = newState ? "ENABLED" : "DISABLED";
    DEBUG_PRINTF("[Main] SPS30 sensor %s\n", status.c_str());
    
    // Visual feedback
    if (newState) {
        leds.setColor(LEDColors::GREEN);
    } else {
        leds.setColor(LEDColors::RED);
    }
    delay(500);
    leds.off();
    
    // Show message on display
    display.showMessage("SPS30", "Sensor", status, "");
    delay(2000);
    
    // Force immediate sensor read to update display
    readSensorsTask();
}

// Add command handler function
void handleMqttCommand(const JsonDocument& cmd) {
    bool configChanged = false;
    
    if (cmd.containsKey("sps30")) {
        bool enable = cmd["sps30"].as<bool>();
        sensors.setSPS30Enabled(enable);
        DEBUG_PRINTF("[MQTT] SPS30: %s\n", enable ? "ON" : "OFF");
    }
    
    if (cmd.containsKey("led_brightness")) {
        uint8_t brightness = cmd["led_brightness"].as<uint8_t>();
        brightness = constrain(brightness, 0, 255);
        leds.setBrightness(brightness);
        AppConfig::instance().setLedBrightness(brightness);
        configChanged = true;
        DEBUG_PRINTF("[MQTT] LED Brightness: %d\n", brightness);
    }
    
    if (cmd.containsKey("temp_fahrenheit")) {
        bool fahrenheit = cmd["temp_fahrenheit"].as<bool>();
        AppConfig::instance().setImperialTemp(fahrenheit);
        configChanged = true;
        DEBUG_PRINTF("[MQTT] Temperature unit: %s\n", fahrenheit ? "°F" : "°C");
    }
    
    if (cmd.containsKey("pressure_inhg")) {
        bool inHg = cmd["pressure_inhg"].as<bool>();
        AppConfig::instance().setImperialQfe(inHg);
        configChanged = true;
        DEBUG_PRINTF("[MQTT] Pressure unit: %s\n", inHg ? "inHg" : "hPa");
    }
    
    if (configChanged) {
        AppConfig::instance().save();
        display.updateSensorDisplay(sensors.getData(), 
                                   AppConfig::instance().isImperialTemp(), 
                                   AppConfig::instance().isOfflineMode());
    }
}

// ============================================================================
// SETUP
// ============================================================================
void initializeHardware() {
    // Initialize display first for visual feedback during boot
    display.begin();
    display.showLogo();
    
    // Initialize LEDs for status indication
    leds.begin();
    leds.showInit();
    
    // Initialize I2C sensors
    sensors.begin(Pins::I2C_SDA, Pins::I2C_SCL);
    useRemoteSensor = sensors.shouldUseRemoteSensor();
    
    // Log sensor status - uses ternary operators to avoid String creation
    DEBUG_PRINTLN(F("\n[Main] Sensor status:"));
    DEBUG_PRINTF("Has SCD4x: %s\n", sensors.hasSCD4x() ? "YES" : "NO");
    DEBUG_PRINTF("Has BMX280: %s\n", sensors.hasBMX280() ? "YES" : "NO");
    DEBUG_PRINTF("Has AHT20: %s\n", sensors.hasAHT20() ? "YES" : "NO");
    DEBUG_PRINTF("Has SPS30: %s\n", sensors.hasSPS30() ? "YES" : "NO");
    DEBUG_PRINTF("Use remote sensor: %s\n", useRemoteSensor ? "YES" : "NO");
}
void initializeButtons() {
    buttons.begin();
    
    // Right button long press: Calibrate CO2 sensor
    // Uses existing onCalibrateSensor function to avoid duplication
    buttons.onRightLongClick(onCalibrateSensor);
    
    // Mode button long press: Reset WiFi and reboot
    // Uses existing onResetWiFi function to avoid duplication
    buttons.onModeLongClick(onResetWiFi);
    
    // Left button click: Toggle SPS30 sensor
    // Uses existing onToggleSPS30 function to avoid duplication
    buttons.onLeftClick(onToggleSPS30);
    
    DEBUG_PRINTLN(F("[Main] Button handlers configured"));
}

void loadConfiguration() {
    // Load configuration from SPIFFS
    // AppConfig handles all file I/O and JSON parsing internally
    AppConfig::instance().load();
    
    // Apply LED brightness immediately
    // Direct access to cached value, no file I/O needed
    leds.setBrightness(AppConfig::instance().getLedBrightness());
    
    // Restore SPS30 sensor state if sensor is available
    // Check sensor presence first to avoid unnecessary operations
    if (sensors.hasSPS30()) {
        sensors.setSPS30Enabled(AppConfig::instance().isSPS30Enabled());
        DEBUG_PRINTF("[Main] SPS30 restored: %s\n", 
                     AppConfig::instance().isSPS30Enabled() ? "ENABLED" : "DISABLED");
    }
    
    DEBUG_PRINTLN(F("[Main] Configuration loaded and applied"));
}
void handleOfflineModeToggle() {
    // Check if left button is held during boot
    if (!buttons.isLeftPressed()) {
        return;  // Early exit if button not pressed
    }
    
    DEBUG_PRINTLN(F("[Main] Offline mode toggle detected"));
    
    // Show visual feedback
    leds.showWiFiToggle();
    
    // Toggle offline mode state
    bool newOfflineMode = !AppConfig::instance().isOfflineMode();
    AppConfig::instance().setOfflineMode(newOfflineMode);
    AppConfig::instance().save();
    
    // Prepare display messages using const char* to avoid String allocation
    const char* mode = newOfflineMode ? "ON" : "OFF";
    const char* wifi = newOfflineMode ? "WIFI disabled!" : "WIFI enabled!";
    
    // Show status on display
    display.showMessage("Offline", "mode:", mode, wifi);
    
    DEBUG_PRINTF("[Main] Offline mode: %s\n", mode);
    
    // Wait for button release before continuing
    // Prevents accidental multiple toggles
    buttons.waitForLeftRelease();
    
    DEBUG_PRINTLN(F("[Main] Offline mode toggle complete"));
}
void initializeNetwork() {
    // Early return for offline mode - saves ~15KB RAM by not initializing WiFi stack
    if (AppConfig::instance().isOfflineMode()) {
        leds.off();
        WiFi.mode(WIFI_OFF);
        WiFi.forceSleepBegin();
        return;
    }
    
    leds.setColor(LEDColors::BLUE);
    
    // Use fixed stack buffer instead of String - saves heap fragmentation
    char identifier[24];
    snprintf(identifier, sizeof(identifier), "TEHYBUG-CO2-%X", ESP.getChipId());
    
    // Lambda captures by reference to avoid copying - saves stack space
    network.setConfigSaveCallback([](const char* server, const char* user, const char* pass) {
        DEBUG_PRINTLN(F("[Main] Saving new WiFi/MQTT config"));
        DEBUG_PRINTF("Server: %s\nUser: %s\nPass: %s\n", server, user, pass);
        AppConfig::instance().setMqttServer(server);
        AppConfig::instance().setUsername(user);
        AppConfig::instance().setPassword(pass);
        AppConfig::instance().save();
    });
    
    // Pass const char* directly - no String object creation
    network.begin(identifier, AppConfig::instance().getMqttServer(), 
                 AppConfig::instance().getUsername(), AppConfig::instance().getPassword());
    
    // Function pointers instead of std::function - saves ~48 bytes per callback
    network.setWebHandlers(
        getMainPageData,
        saveWebConfig,
        getConfigPage,
        handleMqttCommand
    );
    
    WiFi.mode(WIFI_STA);
}
void setup() {
    Serial.begin(115200);
    delay(100);
    
    DEBUG_PRINTLN(F("\n\n========================================"));
    DEBUG_PRINTLN(F("TeHyBug CO2 Desk Firmware"));
    DEBUG_PRINTF("Version: %s\n", VERSION);
    DEBUG_PRINTF("Core: %s\n", ESP.getCoreVersion().c_str());
    DEBUG_PRINTF("CPU: %u MHz\n", ESP.getCpuFreqMHz());
    DEBUG_PRINTF("Chip ID: %X\n", ESP.getChipId());
    DEBUG_PRINTLN(F("========================================\n"));
    
    initializeHardware();
    initializeButtons();
    loadConfiguration();
    handleOfflineModeToggle();
    initializeNetwork();
    
    // Start sensor reading task
    ticker.add(0, Hardware::SENSOR_READ_INTERVAL, 
               [](void*) { readSensorsTask(); }, nullptr, true);
    
    DEBUG_PRINTLN(F("[Main] Setup complete\n"));
}
// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    // Update scheduled tasks
    ticker.update();
    yield();
    
    // Process button events
    buttons.loop();
    yield();
    
    // Online mode operations
    if (!AppConfig::instance().isOfflineMode()) {
        network.loop();
        network.setDeviceState(
            sensors.isSPS30Enabled(),
            AppConfig::instance().isImperialTemp(),
            AppConfig::instance().isImperialQfe(),
            AppConfig::instance().getLedBrightness()
        );
        network.publishState(sensors.getData(), VERSION);
        delay(50);  // Reduce power consumption
    } else {
        delay(250);  // Lower power in offline mode
    }
}
