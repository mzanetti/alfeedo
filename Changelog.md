# Pull Request: Firmware (mzanetti/alfeedo)

## Title
Add timer API endpoints, reset endpoint and extended status fields

## Description

This PR adds several new API endpoints and extends the existing `/api/status` 
response to support richer Home Assistant integration.

### New endpoints

**GET /api/timers**
Returns the list of scheduled timers stored on the device:
```json
{
  "maxTimers": 10,
  "timers": [
    {"id": 0, "time": 420, "mode": "meal"},
    {"id": 1, "time": 720, "mode": "meal"}
  ]
}
```
Time is expressed in minutes since midnight (e.g. 420 = 07:00).

**POST /api/timers**
Adds a new timer:
```json
{"time_h_m": "07:00", "mode": "meal"}
```

**DELETE /api/timers?timer_id=0**
Deletes a timer by ID.

**POST /api/reset**
Triggers a remote restart of the ESP32.

### Extended /api/status fields
The following diagnostic fields were added to the existing `/api/status` response:
- `wifiRssi` — WiFi signal strength (dBm)
- `wifiSsid` — connected WiFi network name
- `ipAddress` — device IP address
- `uptime` — device uptime in seconds
- `freeHeap` — free heap memory in bytes
- `firmwareVersion` — current firmware version string

### Related PR
These changes are required by the companion HA integration PR:
https://github.com/mzanetti/ha-alfeedo/pull/[NUMMER]

### Testing
Tested on physical hardware with the ESP32-based feeder. All endpoints return
correct responses and timers persist across HA restarts.

---

# Pull Request: HA Integration (mzanetti/ha-alfeedo)

## Title
Add timer management, diagnostic sensors, motor/fill settings and card UI improvements

## Description

This PR extends the Home Assistant integration with timer management, 
additional sensors and UI improvements to the custom card.

### New features

**Timer management**
- New `sensor.alfeedo_timers` entity showing the number of active timers,
  with the full timer list (id, time, mode) as attributes
- Timer UI embedded in the custom card: view, add and delete timers directly
  from the dashboard
- HA services `alfeedo.add_timer` (time, mode) and `alfeedo.delete_timer` (timer_id)
  for use in automations

**Diagnostic sensors**
- WiFi signal strength, SSID, IP address, uptime, free heap memory, firmware version
- All marked as `EntityCategory.DIAGNOSTIC`

**Motor and fill sensor settings**
- Number sliders for meal size, snack size and motor speed
- Number sliders for full/empty fill sensor calibration values
- Dynamic min/max values read from the ESP32 API

**Reset button**
- `button.alfeedo_restart_feeder` to remotely reboot the ESP32

### Bug fixes / improvements
- Lovelace resource URL now includes version number from `manifest.json`
  to automatically bust the browser cache on updates
- Old resource entries with a different version are automatically cleaned up on startup
- `cache_headers` set to `False` to prevent stale JS being served after updates

### Related PR
Requires firmware changes from:
https://github.com/mzanetti/alfeedo/pull/[NUMMER]

### Testing
Tested on physical hardware. All entities, services and card UI functions
work as expected.
