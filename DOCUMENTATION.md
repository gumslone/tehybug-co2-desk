# TeHyBug CO2 Desk Firmware Documentation

## Table of Contents

1. [Overview](#overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Software Dependencies](#software-dependencies)
4. [Project Structure](#project-structure)
5. [Configuration](#configuration)
6. [Module Documentation](#module-documentation)
7. [API Reference](#api-reference)
8. [Build Instructions](#build-instructions)
9. [Usage Guide](#usage-guide)
10. [Troubleshooting](#troubleshooting)

---

## Overview

TeHyBug CO2 Desk Firmware is a modular ESP8266-based firmware for indoor air quality monitoring. It supports multiple sensors, an e-paper display, NeoPixel LED indicators, and integrates with Home Assistant via MQTT.

### Features

- **Multi-sensor support**: CO2, PM2.5, temperature, humidity, pressure, altitude
- **E-Paper display**: Low-power 1.54" display with partial refresh
- **LED indicators**: Dual NeoPixel LEDs for CO2 and PM2.5 air quality status
- **Home Assistant integration**: MQTT auto-discovery for seamless setup
- **Web interface**: Configuration and monitoring via built-in web server
- **OTA updates**: Over-the-air firmware updates
- **Offline mode**: Standalone operation without WiFi
- **Remote sensor support**: Fetch data from other TeHyBug devices

---

## Hardware Requirements

### Microcontroller
- **ESP8266** (NodeMCU, Wemos D1 Mini, or compatible)
- Minimum 4MB flash recommended

### Supported Sensors

| Sensor | Type | I2C Address | Required |
|--------|------|-------------|----------|
| SCD4x (SCD40/SCD41) | CO2, Temperature, Humidity | 0x62 | Recommended |
| BME280 | Temperature, Humidity, Pressure | 0x76 or 0x77 | Optional |
| BMP280 | Temperature, Pressure | 0x76 or 0x77 | Optional |
| AHT20 | Temperature, Humidity | 0x38 | Optional |
| SPS30 | Particulate Matter (PM2.5) | 0x69 | Optional |

### Display
- **1.54" E-Paper Display** (DEPG0150BN2 or compatible)
- Resolution: 200x200 pixels
- SPI interface

### LEDs
- **2x WS2812B NeoPixel LEDs**
- LED 0: PM2.5 indicator (or CO2 if no PM sensor)
- LED 1: CO2 indicator

### Pin Configuration

| Function | GPIO | Notes |
|----------|------|-------|
| I2C SDA | GPIO0 | All sensors |
| I2C SCL | GPIO2 | All sensors |
| LED Strip | GPIO12 | WS2812B data |
| Display CS | GPIO16 | SPI chip select |
| Display DC | GPIO15 | Data/Command |
| Button Left | GPIO5 | Input with pullup |
| Button Right | GPIO4 | Input with pullup |
| Button Mode | GPIO0 | Input with pullup (shared with SDA) |

---

## Software Dependencies

### Arduino Libraries

```
- ESP8266 Arduino Core (>= 3.0.0)
- ArduinoJson (>= 6.0.0)
- PubSubClient (MQTT)
- WiFiManager
- Adafruit_NeoPixel
- Adafruit_GFX
- GxEPD (E-Paper library)
- Button2
- TickerScheduler
- SparkFun_SCD4x_Arduino_Library
- ErriezBMX280
- AHT20 (Arduino library)
- SensirionI2cSps30
```

### Platform
- Arduino IDE 1.8.x or 2.x
- PlatformIO (alternative)

---

## Project Structure

```
tehybug_co2_desk_firmware/
├── tehybug_co2_desk_firmware.ino   # Main entry point
├── src/
│   ├── AppConfig.h                 # Configuration singleton
│   ├── sensors/
│   │   ├── ISensor.h               # Sensor interface
│   │   ├── SensorManager.h         # Sensor orchestration
│   │   ├── BMX280Sensor.h          # BME280/BMP280 driver
│   │   ├── AHT20Sensor.h           # AHT20 driver
│   │   ├── SCD4xSensor.h           # SCD40/SCD41 driver
│   │   └── SPS30Sensor.h           # SPS30 PM sensor driver
│   ├── display/
│   │   └── DisplayManager.h        # E-Paper display handler
│   ├── led/
│   │   └── LEDController.h         # NeoPixel LED controller
│   ├── input/
│   │   └── ButtonManager.h         # Button input handler
│   ├── network/
│   │   └── NetworkManager.h        # WiFi, MQTT, Web Server, OTA
│   └── calibration/
│       └── SensorCalibration.h     # SCD4x calibration routines
├── images.h                        # Bitmap assets (logo, icons)
├── Webinterface.h                  # HTML for web configuration
├── DOCUMENTATION.md                # This file
└── README.md                       # Quick start guide
```

---

## Configuration

### AppConfig (src/AppConfig.h)

Configuration is stored in SPIFFS as `/config.json`:

```json
{
  "mqtt_server": "mqtt.example.com",
  "username": "user",
  "password": "pass",
  "imperial_temp": false,
  "imperial_qfe": false,
  "imperial_alt": false,
  "offline_mode": false,
  "led_brightness": 200
}
```

### Configuration Methods

```cpp
AppConfig::instance().load();           // Load from SPIFFS
AppConfig::instance().save();           // Save to SPIFFS
AppConfig::instance().reset();          // Delete config file

// Getters
const char* getMqttServer();
const char* getUsername();
const char* getPassword();
bool isImperialTemp();
bool isOfflineMode();
uint8_t getLedBrightness();

// Setters
void setMqttServer(const char* server);
void setOfflineMode(bool value);
void setLedBrightness(uint8_t value);
```

---

## Module Documentation

### SensorManager

Manages all connected sensors and provides unified data access.

**Features:**
- Automatic I2C bus scanning
- Dynamic sensor initialization
- Unified JSON data output
- Imperial unit conversion

**Key Methods:**

```cpp
void begin(int sdaPin = 0, int sclPin = 2);  // Initialize I2C and sensors
void readAll();                               // Read all sensors
DynamicJsonDocument& getData();               // Get sensor data as JSON
float getCO2();                               // Get CO2 value (-1 if unavailable)
float getPM25();                              // Get PM2.5 value (-1 if unavailable)
float getTemperature();                       // Get temperature (NAN if unavailable)
float getHumidity();                          // Get humidity (NAN if unavailable)
bool hasSCD4x();                              // Check if CO2 sensor available
bool hasPM25();                               // Check if PM sensor available
uint8_t getSensorCount();                     // Number of active sensors
```

### DisplayManager

Handles the e-paper display with automatic partial/full refresh management.

**Features:**
- Automatic ghosting prevention (full refresh every 30 updates)
- Rate limiting (minimum 1 second between updates)
- Multiple layout modes (CO2 only, CO2+PM2.5, etc.)

**Key Methods:**

```cpp
bool begin();                                 // Initialize display
void showLogo();                              // Display startup logo
void showMessage(line1, line2, ...);          // Show multi-line message
void showCenteredText(text, font);            // Show centered text
void updateSensorDisplay(data, imperial, offline);  // Update with sensor data
void forceFullRefresh();                      // Force full display refresh
void clear();                                 // Clear display
bool isReady();                               // Check if display operational
```

### LEDController

Controls dual NeoPixel LEDs for air quality indication.

**LED Assignment:**
- LED 0: PM2.5 indicator (or CO2 if PM2.5 unavailable)
- LED 1: CO2 indicator

**Color Thresholds:**

| Level | CO2 (ppm) | PM2.5 (µg/m³) | Color |
|-------|-----------|---------------|-------|
| Good | < 1000 | < 12 | Green |
| Moderate | 1000-1500 | 12-35 | Yellow |
| Poor | > 1500 | > 35 | Red |

**Key Methods:**

```cpp
void begin();                                 // Initialize LEDs
void setBrightness(uint8_t brightness);       // Set brightness (0-255)
void updateCO2Indicator(float co2);           // Update CO2 LED only
void updatePM25Indicator(float pm25);         // Update PM2.5 LED only
void updateCO2IndicatorBothLEDs(float co2);   // Both LEDs show CO2
void updateAirQualityIndicators(pm25, co2);   // Update both indicators
void setColor(uint32_t color);                // Set all LEDs to color
void off();                                   // Turn off all LEDs
```

### NetworkManager

Handles all network functionality including WiFi, MQTT, web server, and OTA.

**Features:**
- WiFiManager captive portal for initial setup
- MQTT with Home Assistant auto-discovery
- Built-in web server for configuration
- OTA (Over-The-Air) firmware updates
- mDNS support (tehybug.local)

**MQTT Topics:**

```
tehybug-co2-sensor/{device_id}/status   # Online/Offline availability
tehybug-co2-sensor/{device_id}/state    # Sensor data JSON
tehybug-co2-sensor/{device_id}/command  # Commands (reserved)
```

**Key Methods:**

```cpp
void begin(deviceId, mqttServer, user, pass);  // Initialize network
void loop();                                    // Process network events
void publishState(sensorData, version);         // Publish to MQTT
void resetWiFiAndReboot();                      // Reset WiFi settings
bool isConnected();                             // WiFi connection status
bool isMQTTConnected();                         // MQTT connection status
void httpGet(url, callback);                    // HTTP GET request
```

### ButtonManager

Handles physical button inputs with click and long-press detection.

**Button Functions:**
- **Left**: Toggle offline mode (hold at boot)
- **Right**: Trigger CO2 calibration (long press)
- **Mode**: Reset WiFi settings (15s long press)

**Key Methods:**

```cpp
void begin();                                 // Initialize buttons
void loop();                                  // Process button events
void onLeftClick(callback);                   // Set left click handler
void onLeftLongClick(callback);               // Set left long press handler
void onRightClick(callback);
void onRightLongClick(callback);
void onModeClick(callback);
void onModeLongClick(callback);
bool isLeftPressed();                         // Check button state
void waitForLeftRelease();                    // Block until released
```

### SensorCalibration

Provides CO2 sensor calibration routines.

**Calibration Process:**
1. Place sensor outdoors (fresh air ~400 ppm CO2)
2. Wait 5 minutes for equilibration
3. Perform forced recalibration to 400 ppm reference

**Key Methods:**

```cpp
static void calibrateSCD4x(display, onStart, onEnd);
```

---

## API Reference

### Sensor Data JSON Format

```json
{
  "temp": "23.5",
  "temp_imp": "74.3",
  "humi": "45.2",
  "co2": "650",
  "pm25": "8",
  "qfe": "1013.2",
  "alt": "125.3",
  "temp2": "24.1",
  "humi2": "44.8"
}
```

### Web Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Sensor data as JSON |
| `/config` | GET | Configuration page HTML |
| `/config` | POST | Save configuration |
| `/update` | GET/POST | OTA firmware update |

### Home Assistant Auto-Discovery

The firmware automatically publishes Home Assistant MQTT discovery messages for:
- Temperature (°C/°F)
- Humidity (%RH)
- CO2 (ppm)
- PM2.5 (µg/m³)
- Pressure (hPa)
- Altitude (m)
- WiFi RSSI (dBm)

---

## Build Instructions

### Arduino IDE

1. Install ESP8266 board support:
   - File → Preferences → Additional Board URLs:
   - `https://arduino.esp8266.com/stable/package_esp8266com_index.json`

2. Install required libraries via Library Manager

3. Select board:
   - Board: "NodeMCU 1.0" or "LOLIN(WEMOS) D1 mini"
   - Flash Size: "4MB (FS:2MB OTA:~1019KB)"
   - CPU Frequency: "80 MHz"

4. Upload SPIFFS data (for web interface):
   - Tools → ESP8266 Sketch Data Upload

5. Compile and upload

### PlatformIO

```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
lib_deps =
    bblanchon/ArduinoJson@^6.21.0
    knolleary/PubSubClient@^2.8
    tzapu/WiFiManager@^0.16.0
    adafruit/Adafruit NeoPixel@^1.11.0
    adafruit/Adafruit GFX Library@^1.11.0
    lennarthennigs/Button2@^2.2.0
    sparkfun/SparkFun SCD4x Arduino Library@^1.1.0
monitor_speed = 115200
```

---

## Usage Guide

### First Boot

1. Power on the device
2. Connect to WiFi AP: `TEHYBUG-CO2-XXXXXX`
3. Configure WiFi credentials and MQTT server
4. Device will reboot and connect

### Offline Mode Toggle

1. Hold **Left button** during boot
2. LEDs flash pink
3. Display shows "Offline mode: ON/OFF"
4. Release button

### CO2 Calibration

1. Place device outdoors in fresh air
2. Long press **Right button** (1 second)
3. Wait 5+ minutes for calibration
4. Calibration complete message appears

### WiFi Reset

1. Long press **Mode button** (15 seconds)
2. Device resets WiFi settings
3. Captive portal appears for reconfiguration

### LED Indicators

| LED State | Meaning |
|-----------|---------|
| Blue (startup) | Connecting to WiFi |
| Green | Good air quality |
| Yellow | Moderate air quality |
| Red | Poor air quality |
| Pink | Mode change |
| Off | Offline mode |

---

## Troubleshooting

### Sensor Not Detected

1. Check I2C wiring (SDA=GPIO0, SCL=GPIO2)
2. Verify sensor I2C address with I2C scanner
3. Check serial output for error messages
4. Ensure proper voltage levels (3.3V)

### Display Not Working

1. Check SPI connections (CS=GPIO16, DC=GPIO15)
2. Verify display model compatibility
3. Check serial output for initialization messages

### WiFi Connection Issues

1. Ensure 2.4GHz network (ESP8266 doesn't support 5GHz)
2. Check WiFi password
3. Try WiFi reset (15s Mode button press)
4. Check serial output for connection status

### MQTT Not Connecting

1. Verify MQTT server address
2. Check username/password
3. Ensure MQTT port 1883 is accessible
4. Check serial output for MQTT errors

### Memory Issues

1. Monitor free heap: `ESP.getFreeHeap()`
2. Reduce `DynamicJsonDocument` sizes if needed
3. Avoid `String` concatenation in loops

### OTA Update Fails

1. Ensure sufficient free flash space
2. Check OTA password (same as device ID)
3. Verify network stability during update

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 10.04.2025 | Apr 2025 | Refactored modular architecture |

---

## License

This project is open source. See LICENSE file for details.

---

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make changes following the existing code style
4. Test thoroughly on hardware
5. Submit a pull request

---

## Support

- GitHub Issues: Report bugs and feature requests
- Serial Monitor: Debug output at 115200 baud

---

*Documentation generated for TeHyBug CO2 Desk Firmware v10.04.2025*
