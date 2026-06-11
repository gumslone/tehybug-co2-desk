
#pragma once

#include "../DebugConfig.h"
#include <functional>
#include <vector>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "../sensors/EnvironmentalCalculations.h"

#define FIRMWARE_PREFIX "tehybug-co2-sensor"
#define AVAILABILITY_ONLINE "online"
#define AVAILABILITY_OFFLINE "offline"

class NetworkManager {
public:
    using ConfigSaveCallback = std::function<void(const char*, const char*, const char*)>;
    
    NetworkManager() : _server(80), _mqttClient(_wifiClient) {}
    
    // Prevent copying
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    
    void begin(const char* identifier, const char* mqttServer, 
            const char* mqttUser, const char* mqttPass) {
        strncpy(_identifier, identifier, sizeof(_identifier) - 1);
        strncpy(_mqttServer, mqttServer, sizeof(_mqttServer) - 1);
        strncpy(_mqttUser, mqttUser, sizeof(_mqttUser) - 1);
        strncpy(_mqttPass, mqttPass, sizeof(_mqttPass) - 1);
        
        DEBUG_PRINTLN(F("\n[Network] ========== Configuration =========="));
        DEBUG_PRINTF("[Network] Identifier: %s\n", _identifier);
        DEBUG_PRINTF("[Network] MQTT Server: '%s' (length: %d)\n", _mqttServer, strlen(_mqttServer));
        DEBUG_PRINTF("[Network] MQTT User: '%s' (length: %d)\n", _mqttUser, strlen(_mqttUser));
        DEBUG_PRINTF("[Network] MQTT Pass: %s (length: %d)\n", 
                    strlen(_mqttPass) > 0 ? "***" : "(empty)", strlen(_mqttPass));
        DEBUG_PRINTLN(F("[Network] ==========================================\n"));
        
        snprintf(_topicAvailability, sizeof(_topicAvailability),
                "%s/%s/availability", FIRMWARE_PREFIX, _identifier);
        snprintf(_topicState, sizeof(_topicState),
                "%s/%s/state", FIRMWARE_PREFIX, _identifier);
        snprintf(_topicCommand, sizeof(_topicCommand),
                "%s/%s/command", FIRMWARE_PREFIX, _identifier);
        
        setupWiFi();
        setupOTA();
        setupMQTT();
        setupMDNS();
        setupWebServer();
        
        DEBUG_PRINTF("[Network] Hostname: %s\n", _identifier);
        DEBUG_PRINTF("[Network] IP: %s\n", WiFi.localIP().toString().c_str());
    }
    
    void loop() {
        checkWiFi();
        _mqttClient.loop();
        yield();
        MDNS.update();
        yield();
        _server.handleClient();
        
        uint32_t currentMillis = millis();
        if (!_mqttClient.connected() && 
            currentMillis - _lastMqttAttempt >= MQTT_RECONNECT_INTERVAL) {
            _lastMqttAttempt = currentMillis;
            DEBUG_PRINTLN(F("[MQTT] Reconnecting..."));
            reconnectMQTT();
        }
    }
    
    void publishState(const DynamicJsonDocument& sensorData, const char* version) {
        if (!_mqttClient.connected()) return;
        
        uint32_t currentMillis = millis();
        if (currentMillis - _lastPublish < PUBLISH_INTERVAL) return;
        _lastPublish = currentMillis;
        
        publishAutoConfig(sensorData, version);
        publishSensorState(sensorData);
    }
    
    String getSensorJson(const DynamicJsonDocument& sensorData, const char* i2cAddresses) {
        DynamicJsonDocument root(1024);
        root["devices"] = i2cAddresses;
        root["ip"] = WiFi.localIP().toString();
        
        JsonObjectConst js = sensorData.as<JsonObjectConst>();
        for (JsonPairConst kv : js) {
            double value = kv.value().as<double>();
            // Round to 1 decimal place
            root[kv.key()] = round(value * 10.0) / 10.0;
        }
        
        String json;
        serializeJson(root, json);
        return json;
    }
    
    void resetWiFiAndReboot() {
        _wifiManager.resetSettings();
        delay(500);
        ESP.eraseConfig();
        delay(3000);
        ESP.restart();
    }
    
    void setConfigSaveCallback(ConfigSaveCallback callback) {
        DEBUG_PRINTLN(F("[Network] Setting config save callback"));
        _configSaveCallback = callback;
        DEBUG_PRINTF("[Network] Callback is now: %s\n", _configSaveCallback ? "SET" : "NULL");
    }
    
    void setWebHandlers(
        std::function<String()> getMainHandler,
        std::function<void(bool, bool, uint8_t)> saveConfigHandler,
        std::function<String()> getConfigPageHandler,
        std::function<void(const JsonDocument&)> commandHandler = nullptr
    ) {
        _getMainHandler = getMainHandler;
        _saveConfigHandler = saveConfigHandler;
        _getConfigPageHandler = getConfigPageHandler;
        _commandHandler = commandHandler;
    }
    
    void httpGet(const String& url, std::function<void(JsonDocument&)> callback) {
        HTTPClient http;
        http.begin(_wifiClient, url);
        http.addHeader("Content-Type", "text/plain");
        http.setTimeout(HTTP_TIMEOUT);
        
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            DynamicJsonDocument json(2048);
            DeserializationError error = deserializeJson(json, http.getStream());
            if (!error && callback) {
                callback(json);
            }
        } else if (httpCode > 0) {
            DEBUG_PRINTF("[HTTP] GET failed, code: %d\n", httpCode);
        } else {
            DEBUG_PRINTF("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }

    void setDeviceState(bool sps30, bool tempF, bool pressureInHg, uint8_t brightness) {
        _sps30Enabled = sps30;
        _tempFahrenheit = tempF;
        _pressureInHg = pressureInHg;
        _ledBrightness = brightness;
    }

private:
    static const uint32_t MQTT_RECONNECT_INTERVAL = 60000;
    static const uint32_t PUBLISH_INTERVAL = 30000;
    static const uint16_t MQTT_PORT = 1883;
    static const uint16_t HTTP_TIMEOUT = 5000;
    
    WiFiManager _wifiManager;
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    ESP8266WebServer _server;
    ESP8266HTTPUpdateServer _httpUpdater;
    
    char _identifier[24];
    char _mqttServer[80];
    char _mqttUser[24];
    char _mqttPass[65];
    
    char _topicAvailability[128];
    char _topicState[128];
    char _topicCommand[128];
    
    uint32_t _lastMqttAttempt = 0;
    uint32_t _lastPublish = 0;
    bool _shouldSaveConfig = false;

    bool _sps30Enabled = false;
    bool _tempFahrenheit = false;
    bool _pressureInHg = false;
    uint8_t _ledBrightness = 200;
    
    ConfigSaveCallback _configSaveCallback;
    std::function<String()> _getMainHandler;
    std::function<void(bool, bool, uint8_t)> _saveConfigHandler;
    std::function<void(const JsonDocument&)> _commandHandler;
    std::function<String()> _getConfigPageHandler;
    
    WiFiManagerParameter* _paramMqttServer = nullptr;
    WiFiManagerParameter* _paramMqttUser = nullptr;
    WiFiManagerParameter* _paramMqttPass = nullptr;
    
    void setupWiFi() {
        DEBUG_PRINTLN(F("\n[WiFi] ========== WiFi Setup =========="));
        DEBUG_PRINTF("[WiFi] Initial MQTT Server: '%s'\n", _mqttServer);
        DEBUG_PRINTF("[WiFi] Initial MQTT User: '%s'\n", _mqttUser);
        
        _wifiManager.setDebugOutput(false);
        _wifiManager.setSaveConfigCallback([this]() { 
            DEBUG_PRINTLN(F("[WiFi] Configuration save callback triggered"));
            _shouldSaveConfig = true; 
        });
        
        _paramMqttServer = new WiFiManagerParameter("server", "MQTT server", _mqttServer, 80);
        _paramMqttUser = new WiFiManagerParameter("user", "MQTT username", _mqttUser, 24);
        _paramMqttPass = new WiFiManagerParameter("pass", "MQTT password", _mqttPass, 65);
        
        _wifiManager.addParameter(_paramMqttServer);
        _wifiManager.addParameter(_paramMqttUser);
        _wifiManager.addParameter(_paramMqttPass);
        
        WiFi.hostname(_identifier);
        
        std::vector<const char*> menu = {"wifi", "exit"};
        _wifiManager.setShowInfoUpdate(false);
        _wifiManager.setShowInfoErase(false);
        _wifiManager.setMenu(menu);
        _wifiManager.setConfigPortalTimeout(300);
        _wifiManager.setCustomHeadElement("<style>button {background-color: #1FA67A;}</style>");
        
        DEBUG_PRINTLN(F("[WiFi] Starting autoConnect..."));
        _wifiManager.autoConnect(_identifier);
        
        DEBUG_PRINTF("[WiFi] After autoConnect - shouldSaveConfig: %s\n", 
                    _shouldSaveConfig ? "true" : "false");
        
        if (_shouldSaveConfig) {
            DEBUG_PRINTLN(F("[WiFi] New configuration detected from portal"));
            DEBUG_PRINTF("[WiFi] Portal MQTT Server: '%s'\n", _paramMqttServer->getValue());
            DEBUG_PRINTF("[WiFi] Portal MQTT User: '%s'\n", _paramMqttUser->getValue());
            DEBUG_PRINTF("[WiFi] Portal MQTT Pass: %s\n", 
                        strlen(_paramMqttPass->getValue()) > 0 ? "***" : "(empty)");
            
            if (_configSaveCallback) {
                DEBUG_PRINTLN(F("[WiFi] Calling config save callback"));
                _configSaveCallback(
                    _paramMqttServer->getValue(),
                    _paramMqttUser->getValue(),
                    _paramMqttPass->getValue()
                );
                
                // Update local copies
                strncpy(_mqttServer, _paramMqttServer->getValue(), sizeof(_mqttServer) - 1);
                strncpy(_mqttUser, _paramMqttUser->getValue(), sizeof(_mqttUser) - 1);
                strncpy(_mqttPass, _paramMqttPass->getValue(), sizeof(_mqttPass) - 1);
                
                DEBUG_PRINTF("[WiFi] Updated MQTT Server: '%s'\n", _mqttServer);
                DEBUG_PRINTF("[WiFi] Updated MQTT User: '%s'\n", _mqttUser);
            }
        } else {
            DEBUG_PRINTLN(F("[WiFi] Using existing configuration"));
        }
        
        DEBUG_PRINTLN(F("[WiFi] Connected"));
        DEBUG_PRINTLN(F("[WiFi] ==========================================\n"));
    }
    
    void setupOTA() {
        ArduinoOTA.onStart([]() { DEBUG_PRINTLN(F("[OTA] Start")); });
        ArduinoOTA.onEnd([]() { DEBUG_PRINTLN(F("\n[OTA] End")); });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            if (total > 0) {
                DEBUG_PRINTF("[OTA] Progress: %u%%\r", (progress * 100) / total);
            }
        });
        ArduinoOTA.onError([](ota_error_t error) {
            DEBUG_PRINTF("[OTA] Error[%u]: ", error);
            switch (error) {
                case OTA_AUTH_ERROR: DEBUG_PRINTLN(F("Auth Failed")); break;
                case OTA_BEGIN_ERROR: DEBUG_PRINTLN(F("Begin Failed")); break;
                case OTA_CONNECT_ERROR: DEBUG_PRINTLN(F("Connect Failed")); break;
                case OTA_RECEIVE_ERROR: DEBUG_PRINTLN(F("Receive Failed")); break;
                case OTA_END_ERROR: DEBUG_PRINTLN(F("End Failed")); break;
            }
        });
        
        ArduinoOTA.setHostname(_identifier);
        ArduinoOTA.setPassword(_identifier);
        ArduinoOTA.begin();
        
        DEBUG_PRINTLN(F("[OTA] Ready"));
    }
    
    /**
     * @brief Initializes and configures the MQTT client connection.
     * 
     * Sets up the MQTT client with the server address, port, keep-alive interval,
     * buffer size, and message callback handler. After configuration, attempts to
     * establish a connection to the MQTT broker.
     * 
     * The function configures:
     * - MQTT server address and port (1883)
     * - Keep-alive interval (10 seconds)
     * - Message buffer size (2048 bytes)
     * - Callback handler for incoming MQTT messages
     * 
     * @note This function should be called during initialization after WiFi is connected.
     * @note The MQTT server address must be set via begin() before calling this function.
     * 
     * @see reconnectMQTT()
     * @see handleMqttMessage()
     */
    void setupMQTT() {
        _mqttClient.setServer(_mqttServer, MQTT_PORT);
        _mqttClient.setKeepAlive(10);
        _mqttClient.setBufferSize(2048);
        _mqttClient.setCallback([this](const char* topic, const uint8_t* payload, unsigned int length) {
            this->handleMqttMessage(topic, payload, length);
        });
        
        reconnectMQTT();
    }
    
    void setupMDNS() {
        MDNS.end();
        if (MDNS.begin("tehybug")) {
            MDNS.addService("http", "tcp", 80);
            MDNS.addServiceTxt("http", "tcp", "mac", WiFi.macAddress().c_str());
            DEBUG_PRINTLN(F("[mDNS] Started: tehybug.local"));
        }
    }

    void setupWebServer() {
        _httpUpdater.setup(&_server, "/update", "TeHyBug", "FreshAirMakesSense");
        
        _server.on("/", HTTP_GET, [this]() {
            _server.sendHeader("Connection", "close");
            if (_getMainHandler) {
                _server.send(200, "application/json", _getMainHandler());
            } else {
                _server.send(200, "text/plain", "OK");
            }
        });
        
        _server.on("/config", HTTP_POST, [this]() {
            if (_saveConfigHandler) {
                bool imperial_temp = _server.hasArg("imperial_temp") && _server.arg("imperial_temp").length() > 0;
                bool imperial_qfe = _server.hasArg("imperial_qfe") && _server.arg("imperial_qfe").length() > 0;
                uint8_t brightness = 200;
                if (_server.hasArg("led_brightness")) {
                    int val = _server.arg("led_brightness").toInt();
                    if (val >= 0 && val <= 255) brightness = (uint8_t)val;
                }
                _saveConfigHandler(imperial_temp, imperial_qfe, brightness);
            }
            _server.sendHeader("Connection", "close");
            _server.send(200, "text/plain", "Configuration saved successfully!");
        });
        
        _server.on("/config", HTTP_GET, [this]() {
            _server.sendHeader("Connection", "close");
            if (_getConfigPageHandler) {
                _server.send(200, "text/html", _getConfigPageHandler());
            } else {
                _server.send(404, "text/plain", "Not found");
            }
        });
        
        _server.begin();
        DEBUG_PRINTLN(F("[WebServer] Started"));
    }
    
    void reconnectMQTT() {
        DEBUG_PRINTLN(F("\n[MQTT] ========== Connection Attempt =========="));
        DEBUG_PRINTF("[MQTT] Client ID: %s\n", _identifier);
        DEBUG_PRINTF("[MQTT] Server: %s:%d\n", _mqttServer, MQTT_PORT);
        DEBUG_PRINTF("[MQTT] User: %s\n", _mqttUser);
        DEBUG_PRINTF("[MQTT] Availability topic: %s\n", _topicAvailability);
        
        // Check if server address is valid
        if (strlen(_mqttServer) == 0) {
            DEBUG_PRINTLN(F("[MQTT] ERROR: MQTT server address is empty!"));
            return;
        }
        
        // Check WiFi connection
        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_PRINTLN(F("[MQTT] ERROR: WiFi not connected!"));
            return;
        }
        
        DEBUG_PRINTF("[MQTT] WiFi IP: %s\n", WiFi.localIP().toString().c_str());
        
        for (uint8_t attempt = 1; attempt <= 3; ++attempt) {
            DEBUG_PRINTF("[MQTT] Connection attempt %d/3...\n", attempt);
            
            bool connected = _mqttClient.connect(
                _identifier,              // client ID
                _mqttUser,               // username
                _mqttPass,               // password
                _topicAvailability,      // will topic
                1,                       // will QoS
                true,                    // will retain
                AVAILABILITY_OFFLINE     // will message
            );
            
            if (connected) {
                DEBUG_PRINTLN(F("[MQTT] ✓ Connected successfully!"));
                
                // Publish availability
                bool pubResult = _mqttClient.publish(_topicAvailability, AVAILABILITY_ONLINE, true);
                DEBUG_PRINTF("[MQTT] Availability publish: %s\n", pubResult ? "SUCCESS" : "FAILED");
                
                // Subscribe to command topic
                bool subResult = _mqttClient.subscribe(_topicCommand);
                DEBUG_PRINTF("[MQTT] Command subscription: %s\n", subResult ? "SUCCESS" : "FAILED");
                DEBUG_PRINTF("[MQTT] Subscribed to: %s\n", _topicCommand);
                
                DEBUG_PRINTLN(F("[MQTT] ==========================================\n"));
                return;
            }
            
            // Connection failed - get error code
            int state = _mqttClient.state();
            DEBUG_PRINTF("[MQTT] ✗ Connection failed, state: %d - ", state);
            
            switch (state) {
                case -4:
                    DEBUG_PRINTLN(F("MQTT_CONNECTION_TIMEOUT - server didn't respond"));
                    break;
                case -3:
                    DEBUG_PRINTLN(F("MQTT_CONNECTION_LOST - network connection broken"));
                    break;
                case -2:
                    DEBUG_PRINTLN(F("MQTT_CONNECT_FAILED - network connection failed"));
                    break;
                case -1:
                    DEBUG_PRINTLN(F("MQTT_DISCONNECTED - client disconnected"));
                    break;
                case 1:
                    DEBUG_PRINTLN(F("MQTT_CONNECT_BAD_PROTOCOL - server doesn't support MQTT v3.1.1"));
                    break;
                case 2:
                    DEBUG_PRINTLN(F("MQTT_CONNECT_BAD_CLIENT_ID - client ID rejected"));
                    break;
                case 3:
                    DEBUG_PRINTLN(F("MQTT_CONNECT_UNAVAILABLE - server unavailable"));
                    break;
                case 4:
                    DEBUG_PRINTLN(F("MQTT_CONNECT_BAD_CREDENTIALS - bad username/password"));
                    break;
                case 5:
                    DEBUG_PRINTLN(F("MQTT_CONNECT_UNAUTHORIZED - client not authorized"));
                    break;
                default:
                    DEBUG_PRINTF("Unknown error code: %d\n", state);
                    break;
            }
            
            if (attempt < 3) {
                DEBUG_PRINTLN(F("[MQTT] Waiting 5 seconds before retry..."));
                delay(5000);
            }
        }
        
        DEBUG_PRINTLN(F("[MQTT] ✗ All connection attempts failed"));
        DEBUG_PRINTLN(F("[MQTT] ==========================================\n"));
    }
        
    void checkWiFi() {
        static uint8_t disconnectCount = 0;
        
        if (WiFi.status() != WL_CONNECTED) {
            disconnectCount++;
            DEBUG_PRINTF("[WiFi] Disconnected, attempt %u\n", disconnectCount);
            
            if (disconnectCount >= 10) {
                DEBUG_PRINTLN(F("[WiFi] Too many failures, restarting..."));
                ESP.restart();
            }
            
            WiFi.reconnect();
            yield();
        } else {
            disconnectCount = 0;
        }
    }
    
    void handleMqttMessage(const char* topic, const uint8_t* payload, unsigned int length) {
        if (strcmp(topic, _topicCommand) != 0) return;
        
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, payload, length);
        
        if (error) {
            DEBUG_PRINTF("[MQTT] Failed to parse command: %s\n", error.c_str());
            return;
        }
        
        DEBUG_PRINTLN(F("[MQTT] Command received:"));
        DEBUG_PRINT_JSON(doc);
        DEBUG_PRINTLN();
        
        if (_commandHandler) {
            _commandHandler(doc);
        }
    }

    void publishAutoConfig(const DynamicJsonDocument& sensorData, const char* version) {
        // Use stack buffer for payload to avoid heap fragmentation
        char payload[1024];
        
        // Create device configuration once and reuse
        StaticJsonDocument<256> deviceDoc;
        StaticJsonDocument<64> idsDoc;
        JsonArray identifiers = idsDoc.to<JsonArray>();
        identifiers.add(_identifier);
        
        deviceDoc["identifiers"] = identifiers;
        deviceDoc["manufacturer"] = F("TeHyBug");
        deviceDoc["model"] = F("TeHyBug Co2 Desktop (FreshAirMakesSense)");
        deviceDoc["name"] = _identifier;
        deviceDoc["sw_version"] = version;
        
        JsonObject device = deviceDoc.as<JsonObject>();
        
        // Use a single reusable document for all configurations
        StaticJsonDocument<768> autoconf;
        
        // Helper lambda to reduce code duplication
        auto publishConfig = [&](const char* component, const char* suffix, 
                                std::function<void(JsonObject&)> configure) {
            autoconf.clear();
            
            // Build topic using String reserve to minimize allocations
            String topic;
            topic.reserve(80);
            topic = F("homeassistant/");
            topic += component;
            topic += '/';
            topic += FIRMWARE_PREFIX;
            topic += '/';
            topic += _identifier;
            topic += '_';
            topic += suffix;
            topic += F("/config");
            
            JsonObject root = autoconf.to<JsonObject>();
            root["device"] = device;
            root["state_topic"] = _topicState;
            root["availability_topic"] = _topicAvailability;
            
            configure(root);
            
            serializeJson(autoconf, payload, sizeof(payload));
            _mqttClient.publish(topic.c_str(), payload, true);
            yield();
        };
        
        // SPS30 switch
        publishConfig("switch", "sps30", [&](JsonObject& root) {
            root["command_topic"] = _topicCommand;
            root["name"] = F("SPS30 Sensor");
            root["value_template"] = F("{{value_json.sps30_enabled}}");
            root["payload_on"] = F("{\"sps30\":true}");
            root["payload_off"] = F("{\"sps30\":false}");
            root["state_on"] = F("true");
            root["state_off"] = F("false");
            root["unique_id"] = String(_identifier) + F("_sps30");
            root["icon"] = F("mdi:air-filter");
            root["entity_category"] = F("config");
        });

        // LED Brightness number control
        publishConfig("number", "led_brightness", [&](JsonObject& root) {
            root["command_topic"] = _topicCommand;
            root["name"] = F("LED Brightness");
            root["value_template"] = F("{{value_json.led_brightness}}");
            root["command_template"] = F("{\"led_brightness\":{{value}}}");
            root["unique_id"] = String(_identifier) + F("_led_brightness");
            root["min"] = 0;
            root["max"] = 255;
            root["step"] = 5;
            root["mode"] = F("slider");
            root["icon"] = F("mdi:brightness-6");
            root["entity_category"] = F("config");
        });

        // Temperature unit switch
        publishConfig("switch", "temp_fahrenheit", [&](JsonObject& root) {
            root["command_topic"] = _topicCommand;
            root["name"] = F("Temperature in Fahrenheit");
            root["value_template"] = F("{{value_json.temp_fahrenheit}}");
            root["payload_on"] = F("{\"temp_fahrenheit\":true}");
            root["payload_off"] = F("{\"temp_fahrenheit\":false}");
            root["state_on"] = F("true");
            root["state_off"] = F("false");
            root["unique_id"] = String(_identifier) + F("_temp_fahrenheit");
            root["icon"] = F("mdi:thermometer");
            root["entity_category"] = F("config");
        });

        // Pressure unit switch
        publishConfig("switch", "pressure_inhg", [&](JsonObject& root) {
            root["command_topic"] = _topicCommand;
            root["name"] = F("Pressure in inHg");
            root["value_template"] = F("{{value_json.pressure_inhg}}");
            root["payload_on"] = F("{\"pressure_inhg\":true}");
            root["payload_off"] = F("{\"pressure_inhg\":false}");
            root["state_on"] = F("true");
            root["state_off"] = F("false");
            root["unique_id"] = String(_identifier) + F("_pressure_inhg");
            root["entity_category"] = F("config");
            root["icon"] = F("mdi:gauge");
        });

        // WiFi sensor
        publishConfig("sensor", "wifi", [&](JsonObject& root) {
            root["name"] = F("WiFi");
            root["value_template"] = F("{{value_json.wifi.rssi}}");
            root["unique_id"] = String(_identifier) + F("_wifi");
            root["unit_of_measurement"] = F("dBm");
            root["icon"] = F("mdi:wifi");
        });
        
        // Other sensors from sensorData
        JsonObjectConst root = sensorData.as<JsonObjectConst>();
        for (JsonPairConst kv : root) {
            const char* key = kv.key().c_str();
            if (strcmp(key, "key") == 0) continue;
            
            publishConfig("sensor", key, [&](JsonObject& root) {
                String k(key);
                root["name"] = keyToName(k);
                
                // Build value_template efficiently
                String vt;
                vt.reserve(32);
                vt = F("{{value_json.");
                vt += key;
                vt += F("}}");
                root["value_template"] = vt;
                
                root["unit_of_measurement"] = keyToUnit(k);
                root["icon"] = keyToIcon(k);
                
                String uid;
                uid.reserve(48);
                uid = _identifier;
                uid += F("_sensor_");
                uid += key;
                root["unique_id"] = uid;
            });
        }
        
        // Derived temperature-class sensors (dew point and heat index, °C and °F)
        struct DerivedSensor {
            const char* suffix;
            const __FlashStringHelper* name;
            const __FlashStringHelper* unit;
            const __FlashStringHelper* icon;
        };
        const DerivedSensor derivedSensors[] = {
            {"dew",     F("Dew Point"),      F("°C"), F("mdi:water-thermometer")},
            {"dew_imp", F("Dew Point (F)"),  F("°F"), F("mdi:water-thermometer")},
            {"hi",      F("Heat Index"),     F("°C"), F("mdi:sun-thermometer")},
            {"hi_imp",  F("Heat Index (F)"), F("°F"), F("mdi:sun-thermometer")},
        };
        for (const DerivedSensor& d : derivedSensors) {
            publishConfig("sensor", d.suffix, [&](JsonObject& root) {
                root["name"] = d.name;
                String vt;
                vt.reserve(32);
                vt = F("{{value_json.");
                vt += d.suffix;
                vt += F("}}");
                root["value_template"] = vt;
                root["unit_of_measurement"] = d.unit;
                root["icon"] = d.icon;
                root["unique_id"] = String(_identifier) + '_' + d.suffix;
                root["device_class"] = F("temperature");
                root["state_class"] = F("measurement");
            });
        }

        // Indoor Air Quality (IAQ)
        publishConfig("sensor", "iaq", [&](JsonObject& root) {
            root["name"] = F("Indoor Air Quality");
            root["value_template"] = F("{{value_json.iaq}}");
            root["icon"] = F("mdi:air-filter");
            root["unique_id"] = String(_identifier) + F("_iaq");
            root["state_class"] = F("measurement");
        });
    }
    void publishSensorState(const DynamicJsonDocument& sensorData) {
        DynamicJsonDocument state(1024);
        DynamicJsonDocument wifi(192);
        char payload[1024];
        
        wifi["ssid"] = WiFi.SSID();
        wifi["ip"] = WiFi.localIP().toString();
        wifi["rssi"] = WiFi.RSSI();
        state["wifi"] = wifi.as<JsonObject>();

        // Add device settings to state
        state["sps30_enabled"] = _sps30Enabled;
        state["temp_fahrenheit"] = _tempFahrenheit;
        state["pressure_inhg"] = _pressureInHg;
        state["led_brightness"] = _ledBrightness;

        // Extract sensor values for calculations
        double temp = 0.0;
        double humi = 0.0;
        double co2 = 0.0;
        double pm25 = 0.0;
        bool hasTemp = false;
        bool hasHumi = false;
        bool hasCO2 = false;
        bool hasPM25 = false;
        
        JsonObjectConst root = sensorData.as<JsonObjectConst>();
        for (JsonPairConst kv : root) {
            String k = kv.key().c_str();
            if (k == "key") continue;

            // String values pass through verbatim, numeric values get rounded
            double value;
            if (kv.value().is<const char*>()) {
                state[k] = kv.value().as<const char*>();
                value = atof(kv.value().as<const char*>());
            } else {
                value = kv.value().as<double>();
                state[k] = round(value * 10.0) / 10.0;
            }

            // Extract values for derived-metric calculations
            if (k == "temp") {
                temp = value;
                hasTemp = true;
            } else if (k == "humi") {
                humi = value;
                hasHumi = true;
            } else if (k == "co2") {
                co2 = value;
                hasCO2 = true;
            } else if (k == "pm25") {
                pm25 = value;
                hasPM25 = true;
            }
        }
        
        // Calculate and add derived metrics using EnvironmentalCalculations
        if (hasTemp && hasHumi) {
            // Dew Point - always calculate, don't use sensor value
            double dewPoint = EnvironmentalCalculations::calculateDewPoint(temp, humi);
            if (!isnan(dewPoint)) {
                state["dew"] = round(dewPoint * 10.0) / 10.0;
                state["dew_imp"] = round(EnvironmentalCalculations::celsiusToFahrenheit(dewPoint) * 10.0) / 10.0;
            }
            
            // Heat Index - only publish if conditions warrant it (temp >= 27°C, humidity >= 40%)
            if (temp >= 27.0 && humi >= 40.0) {
                double heatIndex = EnvironmentalCalculations::calculateHeatIndex(temp, humi);
                if (!isnan(heatIndex)) {
                    state["hi"] = round(heatIndex * 10.0) / 10.0;
                    state["hi_imp"] = round(EnvironmentalCalculations::celsiusToFahrenheit(heatIndex) * 10.0) / 10.0;
                }
            }
        }
        
        // Indoor Air Quality (IAQ) score
        if (hasTemp && hasHumi && (hasCO2 || hasPM25)) {
            double iaq = EnvironmentalCalculations::calculateIAQ(
                hasCO2 ? co2 : 0.0,
                hasPM25 ? pm25 : 0.0,
                temp,
                humi
            );
            state["iaq"] = round(iaq * 10.0) / 10.0;
        }
        
        serializeJson(state, payload);
        _mqttClient.publish(_topicState, payload, true);
    }
    static String keyToUnit(const String& key) {
        // Use F() macro to store strings in flash memory instead of RAM
        if (key == "temp" || key == "temp2" || key == "dew" || key == "hi") return F("°C");
        if (key.indexOf("_imp") >= 0) return F("°F");
        if (key == "humi" || key == "humi2") return F("%RH");
        if (key == "qfe") return F("hPa");
        if (key == "alt") return F("m");
        if (key == "pm25") return F("µg/m³");
        if (key == "co2") return F("ppm");
        return F(" ");
    }

    static String keyToName(const String& key) {
        // Use F() macro for all string literals
        if (key == "temp") return F("Temperature");
        if (key == "temp2") return F("Temperature 2");
        if (key == "humi") return F("Humidity");
        if (key == "humi2") return F("Humidity 2");
        if (key == "qfe") return F("Pressure");
        if (key == "alt") return F("Altitude");
        if (key == "pm25") return F("PM2.5");
        if (key == "co2") return F("CO2");
        if (key == "dew") return F("Dew Point");
        if (key == "hi") return F("Heat Index");
        if (key.indexOf("_imp") >= 0) return key.substring(0, key.indexOf("_imp")) + F(" (Imperial)");
        return key;
    }

    static String keyToIcon(const String& key) {
        // Use F() macro for all icon strings
        if (key == "temp" || key == "temp2") return F("mdi:thermometer");
        if (key == "humi" || key == "humi2") return F("mdi:water-percent");
        if (key == "qfe") return F("mdi:gauge");
        if (key == "alt") return F("mdi:altimeter");
        if (key == "pm25") return F("mdi:blur");
        if (key == "co2") return F("mdi:molecule-co2");
        if (key == "dew") return F("mdi:water-thermometer");
        if (key == "hi") return F("mdi:sun-thermometer");
        return F("mdi:eye");
    }
};