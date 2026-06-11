#pragma once

// Embedded configuration page, served from flash at GET /config.
// All facts below (pins, thresholds, timings, credentials) mirror the
// firmware source; update this page when those change:
//   - LED thresholds .............. src/led/LEDController.h
//   - Button mapping & hold times . src/input/ButtonManager.h + .ino
//   - Display refresh timing ...... src/display/DisplayManager.h + .ino
//   - Calibration sequence ........ src/calibration/SensorCalibration.h
//   - Endpoints & MQTT topics ..... src/network/NetworkManager.h
const char configPage[] PROGMEM = R"=====(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>TeHyBug CO2 Desk &middot; Settings</title>
<style>
:root{--ink:#1a1a1a;--paper:#fdfdf8;--accent:#1FA67A;--dim:#666;--line:#d8d8cf}
*{box-sizing:border-box}
body{font-family:Georgia,'Times New Roman',serif;background:#e9e9e2;color:var(--ink);margin:0;padding:16px}
main{max-width:680px;margin:0 auto}
h1{font-size:1.5em;margin:.2em 0 0}
h1 small{font-size:.55em;color:var(--dim);font-weight:normal}
h2{font-size:1.05em;border-bottom:2px solid var(--ink);padding-bottom:4px;margin:0 0 12px}
section{background:var(--paper);border:1px solid var(--line);box-shadow:3px 3px 0 rgba(0,0,0,.12);padding:18px 20px;margin:16px 0}
p,li,td,th,label{font-size:.92em;line-height:1.5}
.hint{color:var(--dim);font-size:.85em;margin:4px 0 0 26px}
.unit{color:var(--dim)}
code,kbd{font-family:Menlo,Consolas,monospace;font-size:.85em;background:#efefe6;border:1px solid var(--line);padding:1px 5px;border-radius:3px}
kbd{background:var(--ink);color:#fff;border-color:var(--ink)}
table{border-collapse:collapse;width:100%}
th,td{border:1px solid var(--line);padding:5px 8px;text-align:left}
th{background:#f3f3ea}
.sw{display:inline-block;width:.8em;height:.8em;border-radius:50%;vertical-align:-1px;border:1px solid rgba(0,0,0,.25)}
label.opt{display:block;margin:14px 0 0;font-weight:bold}
input[type=checkbox]{transform:scale(1.2);margin-right:8px;accent-color:var(--accent)}
input[type=number]{width:6em;padding:6px;font:inherit;border:1px solid var(--ink);background:#fff}
input[type=submit]{display:block;width:100%;margin-top:18px;padding:12px;font:inherit;font-weight:bold;color:#fff;background:var(--accent);border:none;cursor:pointer}
input[type=submit]:hover{filter:brightness(.93)}
.note{background:#f7f4df;border-left:4px solid #c9b458;padding:8px 12px;font-size:.85em;margin-top:14px}
/* e-paper demo */
#epd{position:relative;width:220px;height:220px;margin:10px auto;background:#f4f4ec;border:6px solid #2b2b2b;border-radius:4px;font-family:Helvetica,Arial,sans-serif;transition:filter .12s}
#epd.part{filter:contrast(1.6)}
#epd.flash{animation:inv .6s steps(2,end)}
@keyframes inv{0%{filter:invert(1)}50%{filter:invert(0)}75%{filter:invert(1)}100%{filter:invert(0)}}
#epd div{position:absolute;color:#111}
#epd .big{font-size:30px;font-weight:bold}
#epd .lbl{font-size:10px;color:#444}
#epd .dot{width:10px;height:10px;background:#111;border-radius:50%;right:8px;bottom:8px}
.demobar{text-align:center;font-size:.85em;color:var(--dim)}
footer{color:var(--dim);font-size:.8em;text-align:center;margin:20px 0}
a{color:var(--accent)}
</style>
</head>
<body>
<main>
<h1>TeHyBug CO2 Desk <small>settings &amp; manual</small></h1>

<section>
<h2>Settings</h2>
<form action="/config" method="POST">
  <label class="opt"><input type="checkbox" name="imperial_temp"> Temperature in &deg;F</label>
  <p class="hint">Shows temperature in Fahrenheit instead of Celsius, both on the
  e-paper display and in the values published to Home Assistant. Example:
  22.4&nbsp;&deg;C becomes 72.3&nbsp;&deg;F.</p>

  <label class="opt"><input type="checkbox" name="imperial_qfe"> Pressure in inHg</label>
  <p class="hint">Marks inches of mercury as your preferred pressure unit in the
  state reported to Home Assistant. Caveat: the e-paper display itself always
  shows pressure in hPa in the current firmware.</p>

  <label class="opt">LED brightness
  <input type="number" value="200" min="0" max="255" name="led_brightness">
  <span class="unit">0&ndash;255</span></label>
  <p class="hint">Brightness of the two air-quality indicator LEDs.
  Default is 200; 0 switches the LEDs off entirely (the display keeps working).
  Takes effect immediately after saving.</p>

  <input type="submit" value="Save settings">
  <p class="note">This form starts from defaults, not from the values currently
  stored on the device &mdash; submitting overwrites all three settings.
  Settings survive reboots and power loss.</p>
</form>
</section>

<section>
<h2>About this device</h2>
<p>The TeHyBug CO2 Desk is a desk air-quality monitor: an ESP8266 with a
1.54&Prime; 200&times;200 e-paper display and two RGB indicator LEDs. It auto-detects
the sensors connected to its I&sup2;C bus on boot:</p>
<table>
<tr><th>Sensor</th><th>Measures</th><th>I&sup2;C address</th></tr>
<tr><td>SCD4x (SCD40/41)</td><td>CO2, temperature, humidity</td><td><code>0x62</code></td></tr>
<tr><td>BME280 / BMP280</td><td>Pressure, temperature (+humidity on BME)</td><td><code>0x76</code>/<code>0x77</code></td></tr>
<tr><td>AHT20</td><td>Temperature, humidity</td><td><code>0x38</code></td></tr>
<tr><td>SPS30</td><td>Particulate matter (PM2.5)</td><td><code>0x69</code></td></tr>
</table>
<p>Sensors are read every ~10 seconds. If no local sensor is found, the device
fetches readings from another TeHyBug at <code>http://tehybug.local</code>.
With an MQTT broker configured it announces itself to Home Assistant via MQTT
auto-discovery &mdash; sensors, dew point, heat index and an indoor-air-quality
score appear automatically.</p>
<table>
<tr><th>URL on this device</th><th>What it does</th></tr>
<tr><td><code>/</code></td><td>Live sensor readings as JSON</td></tr>
<tr><td><code>/config</code></td><td>This page</td></tr>
<tr><td><code>/update</code></td><td>Firmware update over WiFi (user <code>TeHyBug</code>; password in the project README)</td></tr>
</table>
</section>

<section>
<h2>Indicator LEDs</h2>
<p>The left LED shows particulate matter, the right LED shows CO2. If the
device has no PM sensor (or it is switched off), both LEDs show the CO2
level.</p>
<table>
<tr><th>Color</th><th>CO2 (right LED)</th><th>PM2.5 (left LED)</th></tr>
<tr><td><span class="sw" style="background:#0c0"></span> Green</td><td>up to 1000 ppm</td><td>up to 12 &micro;g/m&sup3;</td></tr>
<tr><td><span class="sw" style="background:#fc0"></span> Yellow</td><td>1001&ndash;1500 ppm</td><td>13&ndash;35 &micro;g/m&sup3;</td></tr>
<tr><td><span class="sw" style="background:#e33"></span> Red</td><td>above 1500 ppm</td><td>above 35 &micro;g/m&sup3;</td></tr>
<tr><td><span class="sw" style="background:#06f"></span> Blue</td><td colspan="2">starting up / connecting to WiFi</td></tr>
<tr><td><span class="sw" style="background:#f0f"></span> Pink</td><td colspan="2">offline-mode toggle was detected at boot</td></tr>
<tr><td><span class="sw" style="background:linear-gradient(90deg,#0c0 50%,#e33 50%)"></span> Green/red blink</td><td colspan="2">PM sensor switched on / off via the left button</td></tr>
</table>
</section>

<section>
<h2>Hardware buttons</h2>
<table>
<tr><th>Button</th><th>Action</th><th>Effect</th></tr>
<tr><td><kbd>IO_5</kbd> (left)</td><td>short press</td><td>Toggle the SPS30 particulate sensor on/off. LEDs blink green (on) or red (off) and the display confirms. The choice is saved.</td></tr>
<tr><td><kbd>IO_5</kbd> (left)</td><td>hold while powering on, release when LEDs turn pink</td><td>Toggle offline mode: WiFi is switched fully off (or back on) and the device runs display-only. The WiFi dot in the display corner disappears in offline mode.</td></tr>
<tr><td><kbd>IO_4</kbd> (right)</td><td>hold ~1 second</td><td>Start CO2 calibration. Put the device outdoors in fresh air first! It takes 5 warm-up readings, settles for 5 minutes, then sets the current air as the 400&nbsp;ppm reference and shows the applied correction (&asymp;6 minutes total).</td></tr>
<tr><td><kbd>Mode</kbd></td><td>hold 15 seconds</td><td>Factory-reset WiFi: stored WiFi and portal settings are erased and the device reboots into its <code>TEHYBUG-CO2-&hellip;</code> setup hotspot.</td></tr>
<tr><td><kbd>Mode</kbd> + <kbd>Reset</kbd></td><td>hold Mode, tap Reset</td><td>Boot into serial flashing mode (for flashing firmware over USB).</td></tr>
<tr><td><kbd>Reset</kbd></td><td>tap</td><td>Reboot.</td></tr>
</table>
</section>

<section>
<h2>Why does the screen flash?</h2>
<p>E-paper keeps its image without power, but pixels slowly &ldquo;ghost&rdquo;.
The firmware therefore mixes two update types: quiet <em>partial updates</em>
(at most one per second) after each ~10-second sensor reading, and a
full-screen <em>refresh flash</em> every 30th update &mdash; roughly every
5 minutes &mdash; which momentarily inverts the panel to wipe ghosting.
That black flicker is normal and keeps the display crisp.</p>
<div id="epd">
  <div class="big" style="left:10px;top:14px"><span id="d-temp">22.4</span><span style="font-size:12px">&deg;C</span></div>
  <div class="big" style="right:10px;top:14px"><span id="d-humi">47</span><span style="font-size:12px">%RH</span></div>
  <div class="big" style="left:10px;top:92px"><span id="d-pm">8</span></div>
  <div class="lbl" style="left:10px;top:128px">PM2.5</div>
  <div class="big" style="right:10px;top:92px"><span id="d-co2">634</span></div>
  <div class="lbl" style="right:10px;top:128px">CO2 PPM</div>
  <div class="big" style="left:10px;bottom:14px;font-size:22px">1013<span style="font-size:11px"> hPa</span></div>
  <div class="dot" title="WiFi on"></div>
</div>
<p class="demobar">Live demo at 30&times; speed &mdash; partial update
<span id="d-cnt">0</span>/30, then a full refresh flash.
Real timing: one reading every 10.033&nbsp;s, full refresh every 30 updates.</p>
<script>
// Real firmware constants: Hardware::SENSOR_READ_INTERVAL,
// DisplayConfig::FULL_REFRESH_INTERVAL (see DisplayManager.h), demoed 30x faster.
var READ_MS = 10033, FULL_EVERY = 30, SPEED = 30, n = 0;
var epd = document.getElementById('epd');
function jiggle(id, base, spread, dec) {
  var v = base + (Math.random() - 0.5) * spread;
  document.getElementById(id).textContent = dec ? v.toFixed(1) : Math.round(v);
}
setInterval(function () {
  n++;
  jiggle('d-temp', 22.4, 0.4, true);
  jiggle('d-humi', 47, 2, false);
  jiggle('d-pm', 8, 2, false);
  jiggle('d-co2', 640, 30, false);
  document.getElementById('d-cnt').textContent = n % FULL_EVERY;
  if (n % FULL_EVERY === 0) {
    epd.className = 'flash';
    setTimeout(function () { epd.className = ''; }, 650);
  } else {
    epd.className = 'part';
    setTimeout(function () { epd.className = ''; }, 130);
  }
}, READ_MS / SPEED);
</script>
</section>

<section>
<h2>Home Assistant / MQTT</h2>
<p>The device subscribes to its command topic
<code>tehybug-co2-sensor/&lt;device-id&gt;/command</code> and accepts a JSON
object with any of these keys (all also exposed as HA controls):</p>
<table>
<tr><th>Key</th><th>Type</th><th>Meaning</th></tr>
<tr><td><code>sps30</code></td><td>bool</td><td>PM sensor on/off</td></tr>
<tr><td><code>led_brightness</code></td><td>0&ndash;255</td><td>Indicator LED brightness</td></tr>
<tr><td><code>temp_fahrenheit</code></td><td>bool</td><td>Temperature unit</td></tr>
<tr><td><code>pressure_inhg</code></td><td>bool</td><td>Pressure unit preference</td></tr>
</table>
</section>

<footer>TeHyBug CO2 Desk &mdash; fresh air makes sense.</footer>
</main>
</body>
</html>
)=====";
