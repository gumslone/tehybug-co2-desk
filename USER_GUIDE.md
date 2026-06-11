# TeHyBug CO2 Desk Monitor - User Guide

## Quick Start

### What's in the Box
- TeHyBug CO2 Monitor
- USB-C / Micro-USB cable for power

### Powering On
1. Connect the USB-C cable to a 5V power source (phone charger, USB port, etc.)
2. The display will show the TeHyBug logo
3. LEDs will turn blue during startup

---

## First-Time Setup (WiFi Configuration)

When you first power on your TeHyBug (or after a WiFi reset), it will create its own WiFi network for setup:

### Step 1: Connect to TeHyBug WiFi
1. On your phone or computer, look for a WiFi network named: **`TEHYBUG-CO2-XXXXXX`**
2. Connect to this network (no password required)
3. A configuration page should open automatically
   - If not, open a browser and go to: `192.168.4.1`

### Step 2: Configure WiFi
1. Click **"Configure WiFi"**
2. Select your home WiFi network from the list
3. Enter your WiFi password
4. (Optional) Enter MQTT server details if using Home Assistant
5. Click **"Save"**

### Step 3: Done!
- The device will restart and connect to your WiFi
- The display will show sensor readings
- LEDs will indicate air quality

---

## Understanding the Display

```
                      ┌─MODE─┐               ┌─RESET─┐ 
                     ┌─────────────────────────────────┐ 
  LEFT Back Button → │                                 │ ← RIGHT Back Button
                     │   ┌─────────────────────────┐   │
                     │   │  23.5°C          45%RH  │   │ ← Temperature & Humidity
                     │   │                         │   │
                     │   │    650         650      │   │ ← CO2 (ppm) & PM2.5 (µg/m³)
                     │   │   PM2.5      CO2 PPM    │   │
                     │   │                         │   │
                     │   │  1013 hPa          📶   │   │ ← Pressure & WiFi icon
                     │   └─────────────────────────┘   │
                     │                *                │
                     │               ***               │
                     └─────────────────────────────────┘
```

### Display Values

| Reading | Unit | What it Means |
|---------|------|---------------|
| **Temperature** | °C or °F | Room temperature |
| **Humidity** | %RH | Relative humidity |
| **CO2** | ppm | Carbon dioxide level |
| **PM2.5** | µg/m³ | Fine particulate matter |
| **Pressure** | hPa | Atmospheric pressure |

---

## Understanding the LED Indicators

Your TeHyBug has two LED lights that show air quality at a glance:

### LED Colors

| Color | Meaning |
|-------|---------|
| 🟢 **Green** | Good air quality |
| 🟡 **Yellow** | Moderate - consider ventilating |
| 🔴 **Red** | Poor - open windows! |
| 🔵 **Blue** | Connecting to WiFi |
| 💗 **Pink** | Mode change |

### What the LEDs Show

| LED | Shows | Green | Yellow | Red |
|-----|-------|-------|--------|-----|
| **Left LED** | PM2.5 | < 12 µg/m³ | 12-35 µg/m³ | > 35 µg/m³ |
| **Right LED** | CO2 | < 1000 ppm | 1000-1500 ppm | > 1500 ppm |

> **Note:** If you don't have a PM2.5 sensor, both LEDs show CO2 levels.

---

## Air Quality Guidelines

### CO2 Levels (Carbon Dioxide)

| Level | Range | Recommendation |
|-------|-------|----------------|
| 🟢 Excellent | < 600 ppm | Fresh air, well ventilated |
| 🟢 Good | 600-1000 ppm | Normal indoor air |
| 🟡 Moderate | 1000-1500 ppm | Room getting stuffy, open a window |
| 🔴 Poor | 1500-2000 ppm | Ventilate immediately |
| 🔴 Very Poor | > 2000 ppm | Leave the room and ventilate |

**Typical values:**
- Outdoor air: ~400 ppm
- Well-ventilated room: 400-800 ppm
- Crowded meeting room: 1000-2500 ppm

### PM2.5 Levels (Fine Particles)

| Level | Range | Recommendation |
|-------|-------|----------------|
| 🟢 Good | 0-12 µg/m³ | Air quality is satisfactory |
| 🟡 Moderate | 12-35 µg/m³ | Acceptable for most people |
| 🔴 Unhealthy | 35-55 µg/m³ | Sensitive groups may be affected |
| 🔴 Very Unhealthy | > 55 µg/m³ | Everyone may experience effects |

---

## Button Functions

Your TeHyBug has three buttons plus a reset button:


```
    [MODE]     [RESET]  

    [LEFT]     [RIGHT]
```

### Quick Reference

| Button | Short Press | Long Press | Special |
|--------|-------------|------------|---------|
| **LEFT** | (reserved) | Toggle Offline Mode (at startup) | - |
| **MODE** | (reserved) | Factory Reset (hold 15 seconds) | - |
| **RIGHT** | (reserved) | Calibrate CO2 sensor (hold 1 second) | - |
| **RESET** | Restart device | - | Hold with MODE for flash mode |

### Reset Button Functions

**Quick Restart:**
- Press RESET button briefly
- Device restarts immediately
- All settings are preserved

**Enter Flash Mode (for firmware updates):**
1. **Hold MODE button**
2. **Press RESET button** (while holding MODE)
3. **Release RESET** first
4. **Release MODE**
5. Device enters programming mode (LEDs off)
6. Connect to computer for firmware flashing

---

## Offline Mode

Use offline mode when you don't need WiFi connectivity (saves power, no network required).

### Enable/Disable Offline Mode
1. **Unplug** the device
2. **Hold the LEFT button**
3. **Plug in** while holding the button
4. Wait for pink LED flash
5. **Release** the button
6. Display shows "Offline mode: ON" or "OFF"

### In Offline Mode:
- ✅ Display works normally
- ✅ LED indicators work
- ❌ No WiFi connection
- ❌ No web interface
- ❌ No Home Assistant integration

---

## CO2 Sensor Calibration

Calibrate your CO2 sensor every few months for accurate readings.

### When to Calibrate
- First use after purchase
- If readings seem consistently off
- Every 3-6 months for best accuracy

### How to Calibrate

1. **Take the device outside** (or to a well-ventilated area)
2. **Wait 5 minutes** for the sensor to stabilize
3. **Long press the RIGHT button** (hold for 1 second)
4. Display shows "Calibration started"
5. **Wait 5 minutes** - don't move the device!
6. Display shows "Calibration finished"
7. Bring the device back inside

> **Important:** Outdoor air is approximately 400 ppm CO2. The calibration sets this as the reference point.

---

## WiFi Reset

If you need to connect to a different WiFi network or fix connection issues:

### How to Reset WiFi
1. **Long press the MODE button** for **15 seconds**
2. The device will restart
3. It will create the setup WiFi network again
4. Follow the [First-Time Setup](#first-time-setup-wifi-configuration) steps

> **Warning:** This erases all configuration including WiFi credentials, MQTT settings, and calibration data!

---

## Web Interface

When connected to WiFi, you can access your TeHyBug from any device on the same network:

### Access the Web Interface
1. Open a web browser
2. Go to: **`http://tehybug.local`**
   - Or use the device's IP address

### Available Pages

| URL | Description |
|-----|-------------|
| `http://tehybug.local/` | Current sensor readings (JSON) |
| `http://tehybug.local/config` | Settings page |
| `http://tehybug.local/update` | Firmware update page |

### Settings You Can Change
- Temperature units (°C / °F)
- LED brightness (0-255)
- Pressure units

---

## Home Assistant Integration

TeHyBug automatically integrates with Home Assistant via MQTT.

### Requirements
- Home Assistant with MQTT broker (Mosquitto)
- MQTT server address configured in TeHyBug

### Setup
1. During WiFi setup, enter your MQTT server address
2. Enter username/password if required
3. TeHyBug will automatically appear in Home Assistant

### Available Sensors in Home Assistant
- Temperature
- Humidity  
- CO2
- PM2.5 (if sensor present)
- Pressure
- Altitude
- WiFi Signal Strength

---

## Troubleshooting

### Device won't turn on
- Check USB cable connection
- Try a different USB power source
- Ensure power source provides at least 500mA
- **Press RESET button** to restart


### Display is blank
- Wait 30 seconds after power on
- The display may be in deep sleep - press any button
- Check if LEDs are working (indicates power is OK)
- **Press RESET button** to restart

### Can't find TeHyBug WiFi network
- Wait 30 seconds after power on
- Move closer to the device
- Try resetting WiFi (hold MODE for 15 seconds)
- **Press RESET button** to restart

### Won't connect to my WiFi
- Ensure you're using a 2.4GHz network (5GHz not supported)
- Check that the password is correct
- Move the device closer to your router
- Try resetting WiFi and setting up again
- **Press RESET button** to restart

### Readings seem wrong
- **CO2 too high/low:** Calibrate the sensor outdoors
- **Temperature off:** Sensor may need 15-30 minutes to stabilize
- **Humidity off:** Avoid placing near air vents or heaters

### LEDs not working
- Check LED brightness setting in web interface
- Brightness of 0 = LEDs off

### Can't access web interface
- Ensure you're on the same WiFi network
- Try using IP address instead of tehybug.local
- Check if device is in offline mode

### Need to update firmware manually
1. **Hold MODE button**
2. **Press RESET button** (while holding MODE)
3. **Release RESET**, then **Release MODE**
4. Connect to computer via USB
5. Use ESPTool or Arduino IDE to flash firmware
---

## Specifications

| Feature | Specification |
|---------|---------------|
| Power | 5V DC via USB-C |
| Power consumption | ~0.5W typical |
| WiFi | 2.4GHz 802.11 b/g/n |
| Display | 1.54" E-Paper, 200x200 pixels |
| CO2 range | 0-40,000 ppm |
| CO2 accuracy | ±(40 ppm + 5% of reading) |
| Temperature range | -10°C to +60°C |
| Humidity range | 0-100% RH |
| PM2.5 range | 0-1000 µg/m³ |
| Dimensions | Varies by enclosure |

---

## Safety Information

- **Indoor use only**
- Do not expose to water or high humidity
- Do not block ventilation holes
- Keep away from heat sources
- Use only with 5V USB power sources
- Not a safety device - do not rely on it for life-critical decisions

---

## Tips for Best Results

1. **Placement**
   - Place at breathing height (desk level)
   - Keep away from windows, doors, and air vents
   - Don't place in direct sunlight
   - Allow air to circulate around the sensor

2. **Warm-up Time**
   - CO2 sensor needs ~30 seconds to start
   - Full accuracy after 3 minutes
   - Temperature/humidity stabilize in 15-30 minutes

3. **Ventilation Recommendations**
   - Open windows when CO2 exceeds 1000 ppm
   - In meetings, take breaks to ventilate
   - Consider mechanical ventilation if CO2 is consistently high

4. **Maintenance**
   - Calibrate CO2 sensor every 3-6 months
   - Keep sensor openings clean and unobstructed
   - Update firmware when new versions are available

---

## Support

For help and updates:
- Check the latest documentation online
- Report issues on GitHub
- Community forums

---

**Enjoy cleaner, healthier air with your TeHyBug CO2 Monitor!** 🐞💨
