#pragma once

#include "../DebugConfig.h"
#include <Arduino.h>
#include <Button2.h>
#include <functional>

/**
 * @brief Button pin definitions
 */
namespace ButtonPins {
    const uint8_t LEFT = 5;
    const uint8_t RIGHT = 4;
    const uint8_t MODE = 0;
}

/**
 * @brief Callback types for button events
 */
using ButtonCallback = std::function<void()>;

/**
 * @brief Manages hardware buttons with callbacks
 */
class ButtonManager {
public:
    ButtonManager() = default;
    
    // Prevent copying
    ButtonManager(const ButtonManager&) = delete;
    ButtonManager& operator=(const ButtonManager&) = delete;
    
    void begin() {
        pinMode(ButtonPins::LEFT, INPUT_PULLUP);
        pinMode(ButtonPins::RIGHT, INPUT_PULLUP);
        pinMode(ButtonPins::MODE, INPUT_PULLUP);
        
        // Left button
        _buttonLeft.begin(ButtonPins::LEFT);
        _buttonLeft.setLongClickTime(1000);
        _buttonLeft.setDoubleClickTime(400);
        
        // Right button
        _buttonRight.begin(ButtonPins::RIGHT);
        _buttonRight.setLongClickTime(1000);
        _buttonRight.setDoubleClickTime(400);
        
        // Mode button (longer press for reset)
        _buttonMode.begin(ButtonPins::MODE);
        _buttonMode.setLongClickTime(15000);
        _buttonMode.setDoubleClickTime(400);
        
        DEBUG_PRINTLN(F("[Buttons] Initialized"));
    }
    
    /**
     * @brief Process button events (call in loop)
     */
    void loop() {
        _buttonLeft.loop();
        _buttonRight.loop();
        _buttonMode.loop();
    }
    
    /**
     * @brief Set callback for left button click
     */
    void onLeftClick(ButtonCallback callback) {
        _onLeftClick = callback;
        _buttonLeft.setClickHandler([this](Button2&) {
            if (_onLeftClick) _onLeftClick();
        });
    }
    
    /**
     * @brief Set callback for right button long press
     */
    void onRightLongClick(ButtonCallback callback) {
        _onRightLongClick = callback;
        _buttonRight.setLongClickHandler([this](Button2&) {
            if (_onRightLongClick) _onRightLongClick();
        });
    }
    
    /**
     * @brief Set callback for mode button long press
     */
    void onModeLongClick(ButtonCallback callback) {
        _onModeLongClick = callback;
        _buttonMode.setLongClickHandler([this](Button2&) {
            if (_onModeLongClick) _onModeLongClick();
        });
    }
    
    /**
     * @brief Check if left button is currently pressed
     */
    bool isLeftPressed() const {
        return digitalRead(ButtonPins::LEFT) == LOW;
    }
    
    /**
     * @brief Wait until left button is released
     */
    void waitForLeftRelease() {
        while (isLeftPressed()) {
            delay(1);
        }
    }

private:
    Button2 _buttonLeft;
    Button2 _buttonRight;
    Button2 _buttonMode;
    
    ButtonCallback _onLeftClick;
    ButtonCallback _onRightLongClick;
    ButtonCallback _onModeLongClick;
};
