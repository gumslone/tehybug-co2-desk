#pragma once

#include "./DebugConfig.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>

/**
 * @brief Singleton configuration manager
 * 
 * Handles loading/saving configuration to SPIFFS
 */
class AppConfig {
public:
    static AppConfig& instance() {
        static AppConfig instance;
        return instance;
    }
    
    // Prevent copying
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;
    
    void load() {
        if (!SPIFFS.begin()) {
            DEBUG_PRINTLN(F("[Config] Failed to mount SPIFFS"));
            return;
        }
        
        if (!SPIFFS.exists("/config.json")) {
            DEBUG_PRINTLN(F("[Config] No config file found, using defaults"));
            return;
        }
        
        File configFile = SPIFFS.open("/config.json", "r");
        if (!configFile) {
            DEBUG_PRINTLN(F("[Config] Failed to open config file"));
            return;
        }
        
        // Use fixed buffer instead of dynamic allocation
        char buf[512];
        size_t size = configFile.size();
        if (size >= sizeof(buf)) {
            DEBUG_PRINTLN(F("[Config] Config file too large"));
            configFile.close();
            return;
        }
        configFile.readBytes(buf, size);
        buf[size] = '\0';
        configFile.close();
        
        DynamicJsonDocument json(512);
        if (deserializeJson(json, buf) != DeserializationError::Ok) {
            DEBUG_PRINTLN(F("[Config] Failed to parse config"));
            return;
        }
        
        if (json.containsKey("mqtt_server")) {
            strncpy(_mqttServer, json["mqtt_server"], sizeof(_mqttServer) - 1);
        }
        if (json.containsKey("username")) {
            strncpy(_username, json["username"], sizeof(_username) - 1);
        }
        if (json.containsKey("password")) {
            strncpy(_password, json["password"], sizeof(_password) - 1);
        }
        
        _imperialTemp = json["imperial_temp"] | false;
        _imperialQfe = json["imperial_qfe"] | false;
        _imperialAlt = json["imperial_alt"] | false;
        _offlineMode = json["offline_mode"] | false;
        _ledBrightness = json["led_brightness"] | 200;
        _sps30Enabled = json["sps30_enabled"] | true;
        DEBUG_PRINTLN(F("[Config] Loaded values:"));
        DEBUG_PRINTF("[Config]   Offline mode: %s\n", _offlineMode ? "true" : "false");
        DEBUG_PRINTF("[Config]   Imperial temp: %s\n", _imperialTemp ? "true" : "false");
        DEBUG_PRINTF("[Config]   Imperial QFE: %s\n", _imperialQfe ? "true" : "false");
        DEBUG_PRINTF("[Config]   SPS30 enabled: %s\n", _sps30Enabled ? "true" : "false");
        DEBUG_PRINTF("[Config]   LED brightness: %d\n", _ledBrightness);
        DEBUG_PRINTF("[Config]   MQTT Server: '%s'\n", _mqttServer);
        DEBUG_PRINTF("[Config]   MQTT User: '%s'\n", _username);
        DEBUG_PRINTF("[Config]   MQTT Pass: %s\n", strlen(_password) > 0 ? "***" : "(empty)");
        DEBUG_PRINTLN(F("[Config] ==========================================\n"));
        DEBUG_PRINTLN(F("[Config] Loaded successfully"));
    }
    
    void save() {
        DynamicJsonDocument json(512);
        
        json["mqtt_server"] = _mqttServer;
        json["username"] = _username;
        json["password"] = _password;
        json["imperial_temp"] = _imperialTemp;
        json["imperial_qfe"] = _imperialQfe;
        json["imperial_alt"] = _imperialAlt;
        json["offline_mode"] = _offlineMode;
        json["led_brightness"] = _ledBrightness;
        json["sps30_enabled"] = _sps30Enabled;
        
        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) {
            DEBUG_PRINTLN(F("[Config] Failed to open config file for writing"));
            return;
        }
        
        serializeJson(json, configFile);
        configFile.close();
        
        DEBUG_PRINTLN(F("[Config] Saved successfully"));
    }
    
    void reset() {
        if (SPIFFS.begin() && SPIFFS.exists("/config.json")) {
            SPIFFS.remove("/config.json");
            DEBUG_PRINTLN(F("[Config] Reset to defaults"));
        }
    }
    
    // Getters
    const char* getMqttServer() const { return _mqttServer; }
    const char* getUsername() const { return _username; }
    const char* getPassword() const { return _password; }
    bool isImperialTemp() const { return _imperialTemp; }
    bool isImperialQfe() const { return _imperialQfe; }
    bool isImperialAlt() const { return _imperialAlt; }
    bool isOfflineMode() const { return _offlineMode; }
    uint8_t getLedBrightness() const { return _ledBrightness; }
    
    // Setters
    void setMqttServer(const char* server) { 
        strncpy(_mqttServer, server, sizeof(_mqttServer) - 1); 
    }
    void setUsername(const char* user) { 
        strncpy(_username, user, sizeof(_username) - 1); 
    }
    void setPassword(const char* pass) { 
        strncpy(_password, pass, sizeof(_password) - 1); 
    }
    void setImperialTemp(bool value) { _imperialTemp = value; }
    void setImperialQfe(bool value) { _imperialQfe = value; }
    void setImperialAlt(bool value) { _imperialAlt = value; }
    void setOfflineMode(bool value) { _offlineMode = value; }
    void setLedBrightness(uint8_t value) { _ledBrightness = value; }
    
    // For WiFiManager compatibility
    char* getMqttServerBuffer() { return _mqttServer; }
    char* getUsernameBuffer() { return _username; }
    char* getPasswordBuffer() { return _password; }

    bool isSPS30Enabled() const { return _sps30Enabled; }
    void setSPS30Enabled(bool enabled) { _sps30Enabled = enabled; }

private:
    AppConfig() = default;
    
    char _mqttServer[80] = "example.tld";
    char _username[24] = "";
    char _password[65] = "";
    
    bool _imperialTemp = false;
    bool _imperialQfe = false;
    bool _imperialAlt = false;
    bool _offlineMode = false;
    
    uint8_t _ledBrightness = 200;
    bool _sps30Enabled = true;
};

// Convenience macro
#define Config AppConfig::instance()
