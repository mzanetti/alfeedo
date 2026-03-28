/*
This file is part of alfeedo.

alfeedo is free software: you can redistribute it and/or modify it under the 
terms of the GNU General Public License as published by the Free Software 
Foundation, either version 3 of the License, or (at your option) any later version.

alfeedo is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with alfeedo. 
If not, see <https://www.gnu.org/licenses/>. 

Copyright (C) 2025-2026 Michael Zanetti <michael_zanetti@gmx.net>
*/

#include "CatServer.h"
#include "CatServerFrontend.h"
#include "TimerEntry.h"
#include "Engine.h"
#include "OTAManager.h"
#include "FillSensor.h"
#include "TimeSource.h"
#include "DisplayController.h"
#include "NetworkConfigManager.h"
#include "Logging.h"

LoggingCategory lcWebServer("WebServer");

extern const uint8_t style_css_start[] asm("_binary_data_style_css_gz_start");
extern const uint8_t style_css_end[] asm("_binary_data_style_css_gz_end");

extern const uint8_t logo_svg_start[] asm("_binary_data_logo_svg_gz_start");
extern const uint8_t logo_svg_end[] asm("_binary_data_logo_svg_gz_end");

extern const uint8_t favicon_ico_start[] asm("_binary_data_favicon_ico_gz_start");
extern const uint8_t favicon_ico_end[] asm("_binary_data_favicon_ico_gz_end");

extern const uint8_t index_html_start[] asm("_binary_data_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_data_index_html_end");

extern const uint8_t settings_html_start[] asm("_binary_data_settings_html_start");
extern const uint8_t settings_html_end[] asm("_binary_data_settings_html_end");

extern const uint8_t timers_html_start[] asm("_binary_data_timers_html_start");
extern const uint8_t timers_html_end[] asm("_binary_data_timers_html_end");

extern const uint8_t logs_html_start[] asm("_binary_data_logs_html_start");
extern const uint8_t logs_html_end[] asm("_binary_data_logs_html_end");

extern const uint8_t fillsensorsettings_html_start[] asm("_binary_data_fillsensorsettings_html_start");
extern const uint8_t fillsensorsettings_html_end[] asm("_binary_data_fillsensorsettings_html_end");

extern const uint8_t motorsettings_html_start[] asm("_binary_data_motorsettings_html_start");
extern const uint8_t motorsettings_html_end[] asm("_binary_data_motorsettings_html_end");

extern const uint8_t networksettings_html_start[] asm("_binary_data_networksettings_html_start");
extern const uint8_t networksettings_html_end[] asm("_binary_data_networksettings_html_end");

extern const uint8_t networkchanged_html_start[] asm("_binary_data_networkchanged_html_start");
extern const uint8_t networkchanged_html_end[] asm("_binary_data_networkchanged_html_end");

extern const uint8_t timesettings_html_start[] asm("_binary_data_timesettings_html_start");
extern const uint8_t timesettings_html_end[] asm("_binary_data_timesettings_html_end");

extern const uint8_t firmware_html_start[] asm("_binary_data_firmware_html_start");
extern const uint8_t firmware_html_end[] asm("_binary_data_firmware_html_end");

extern const uint8_t debuglogs_html_start[] asm("_binary_data_debuglogs_html_start");
extern const uint8_t debuglogs_html_end[] asm("_binary_data_debuglogs_html_end");

extern const uint8_t reset_html_start[] asm("_binary_data_reset_html_start");
extern const uint8_t reset_html_end[] asm("_binary_data_reset_html_end");

#define LOAD_EMBED_FILE(name) \
    String name##_str((const char*)name##_start, name##_end - name##_start); \
    if (name##_str.length() > 0 && name##_str[name##_str.length() - 1] == '\0') { \
        name##_str.remove(name##_str.length() - 1); \
    } \
    name##_str.trim();

CatServerFrontend::CatServerFrontend() {
    m_routeHandlers = {
        {"/", &CatServerFrontend::handleRoot},
        {"/logs", &CatServerFrontend::handleLogs},
        {"/settings", &CatServerFrontend::handleSettings},
        {"/settings/time", &CatServerFrontend::handleTimeSettings},
        {"/settings/timers", &CatServerFrontend::handleTimerSettings},
        {"/settings/network", &CatServerFrontend::handleNetworkSettings},
        {"/settings/networkchanged", &CatServerFrontend::handleNetworkChanged},
        {"/settings/motor", &CatServerFrontend::handleMotorSettings},
        {"/settings/fillsensor", &CatServerFrontend::handleFillSensorSettings},
        {"/firmware", &CatServerFrontend::handleFirmware},
        {"/feed", &CatServerFrontend::handleFeed},
        {"/reset", &CatServerFrontend::handleReset},
        {"/debug", &CatServerFrontend::handleDebugLogs}
    };
}   

bool CatServerFrontend::canHandle(AsyncWebServerRequest *request) const {
    return m_routeHandlers.find(request->url()) != m_routeHandlers.end() ||
           request->url() == "/style.css" ||
           request->url() == "/logo.svg" ||
           request->url() == "/favicon.ico";
}

void CatServerFrontend::begin(Engine *engine, FillSensor *fillSensor, OTAManager *otaManager, TimeSource *timeSource, DisplayController *displayController, NetworkConfigManager *networkConfigManager, ResetFunction resetFunction) {
    m_engine = engine;
    m_otaManager = otaManager;
    m_fillSensor = fillSensor;
    m_timeSource = timeSource;
    m_displayController = displayController;
    m_networkConfigManager = networkConfigManager;
    m_resetFunction = resetFunction;
}

void CatServerFrontend::loop() {
    if (m_rebootPending) {
        delay(500);
        ESP.restart();
    }
    if (m_wifiConfigState == WiFiConfigState::Pending) {
        delay(500);
        m_networkConfigManager->configure(m_pendingWiFiMode, m_pendingSSID, m_pendingPassword);
        m_wifiConfigState = WiFiConfigState::Idle;
    }
}

void CatServerFrontend::handleRequest(AsyncWebServerRequest *request) {
    String fullPath = request->url();

    String argList = "";
    for (uint8_t i = 0; i < request->args(); i++) {
        argList += request->argName(i) + "=" + request->arg(i) + "&";
    }
    logDebug(lcWebServer, "Handling request for path: " + fullPath + " Method: " + String((int)request->method()) + " Args: " + String(request->args()) + " [" + argList + "]");

    auto it = m_routeHandlers.find(fullPath);;
    if (it != m_routeHandlers.end()) {
        (this->*(it->second))(request);
        return;
    }

    if (fullPath == "/style.css") {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/css", style_css_start, (style_css_end - style_css_start));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
        return;
    }

    if (fullPath == "/logo.svg") {
        AsyncWebServerResponse *response = request->beginResponse(200, "image/svg+xml", logo_svg_start, (logo_svg_end - logo_svg_start));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
        return;
    }

    if (fullPath == "/favicon.ico") {
        AsyncWebServerResponse *response = request->beginResponse(200, "image/x-icon", favicon_ico_start, (favicon_ico_end - favicon_ico_start));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
        return;
    }

    logWarning(lcWebServer, "No handler found for path: " + fullPath);
}

void CatServerFrontend::handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String fullPath = request->url();
    // Serial.println("Handling body for path: " + fullPath);

    logWarning(lcWebServer, "No body handler found for path: " + fullPath);
}

void CatServerFrontend::handleUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
    String fullPath = request->url();
    // Serial.println("Handling upload for path: " + fullPath + ", file: " + filename + " Index: " + String(index) + " Len: " + String(len) + " Final: " + String(final));

    std::map<String, void (CatServerFrontend::*)(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)> routeHandlers = {
        {"/firmware", &CatServerFrontend::handleFirmwareUpload}
    };

    auto it = routeHandlers.find(fullPath);;
    if (it != routeHandlers.end()) {
        (this->*(it->second))(request, filename, index, data, len, final);
        return;
    }

    logWarning(lcWebServer, "No upload handler found for path: " + fullPath);
}

void CatServerFrontend::sendWrappedResponse(AsyncWebServerRequest *request, int code, const String &body) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->setCode(code);
    response->print("<html>");
    String header = R"(
    <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <link rel="stylesheet" type="text/css" href="/style.css">
    </head>)";
    response->print(header);
    response->print("<body>");
    response->print(body);
    String footer = R"(
        <div class='footer' style='text-align:center; font-size:0.85em; color:#888; margin-top:1.2em;'>
            Firmware: )" + String(CATFEEDER_VERSION) + R"(
        </div>)";
    response->print(footer);
    response->print("</body>");
    response->print("</html>");
    request->send(response);
}

void CatServerFrontend::handleRoot(AsyncWebServerRequest *request) {
    LOAD_EMBED_FILE(index_html);
    sendWrappedResponse(request, 200, index_html_str);
}

void CatServerFrontend::handleLogs(AsyncWebServerRequest *request) {
    logInfo(lcWebServer, "Logs requested");


    if (request->method() == HTTP_DELETE) {
        m_engine->clearLogs();

        request->redirect("/logs", 303);
        return;
    }

    LOAD_EMBED_FILE(logs_html);
    sendWrappedResponse(request, 200, logs_html_str);
}

void CatServerFrontend::handleSettings(AsyncWebServerRequest *request) {
    LOAD_EMBED_FILE(settings_html);
    sendWrappedResponse(request, 200, settings_html_str);
}

void CatServerFrontend::handleNetworkSettings(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_POST) {

        if (request->hasParam("hostname", true)) {
            String hostname = request->arg("hostname");
            logInfo(lcWebServer, "Hostname :" + hostname);
            m_networkConfigManager->setHostname(hostname);
        }

        if (request->hasParam("wifi_mode", true) && request->hasParam("ssid", true) && request->hasParam("pass", true)) {
    
            String mode = request->arg("wifi_mode");
            logInfo(lcWebServer, "Mode :" + mode);
            std::map<String, NetworkConfigManager::WifiMode> modeMap = {
                {"STA", NetworkConfigManager::WifiMode::Client},
                {"AP", NetworkConfigManager::WifiMode::AP}
            };
            m_pendingWiFiMode = modeMap.at(mode);
    
            m_pendingSSID = request->arg("ssid");
            logInfo(lcWebServer, "SSID: " + m_pendingSSID);

            // If still the same value that we gave out, assume unchanged password and reuse existing one.
            m_pendingPassword = request->arg("pass") == "--- hidden ---" ? m_networkConfigManager->pass() : request->arg("pass");
            logInfo(lcWebServer, "Pass: " + m_pendingPassword);

            m_wifiConfigState = WiFiConfigState::Confirming;
        }

        request->redirect("/settings/networkchanged", 303);
        return;
    }

    LOAD_EMBED_FILE(networksettings_html);
    networksettings_html_str.replace("%%SSID%%", m_networkConfigManager->ssid());
    networksettings_html_str.replace("%%HOSTNAME%%", m_networkConfigManager->hostname());
    networksettings_html_str.replace("%%PASS%%", "--- hidden ---");
    networksettings_html_str.replace("%%AP_CHECKED%%", m_networkConfigManager->wifiMode() == NetworkConfigManager::WifiMode::AP ? " checked" : "");
    networksettings_html_str.replace("%%CLIENT_CHECKED%%", m_networkConfigManager->wifiMode() == NetworkConfigManager::WifiMode::Client ? " checked" : "");
    sendWrappedResponse(request, 200, networksettings_html_str);
}

void CatServerFrontend::handleNetworkChanged(AsyncWebServerRequest *request) {
    LOAD_EMBED_FILE(networkchanged_html);
    networkchanged_html_str.replace("%%HOSTNAME%%", m_networkConfigManager->hostname());
    networkchanged_html_str.replace("%%WIFI_NAME%%", m_pendingSSID);
    sendWrappedResponse(request, 200, networkchanged_html_str);

    if (m_wifiConfigState == WiFiConfigState::Confirming) {
        m_wifiConfigState = WiFiConfigState::Pending;
    }
}

void CatServerFrontend::handleMotorSettings(AsyncWebServerRequest *request) {
    LOAD_EMBED_FILE(motorsettings_html);
    motorsettings_html_str.replace("%%SPEED%%", String(m_engine->speed()));
    motorsettings_html_str.replace("%%MIN_SPEED%%", String(m_engine->minSpeed()));
    motorsettings_html_str.replace("%%MAX_SPEED%%", String(m_engine->maxSpeed()));
    motorsettings_html_str.replace("%%REVOLUTIONS_PER_PORTION%%", String(m_engine->revolutionsPerPortion()));
    motorsettings_html_str.replace("%%MIN_REVOLUTIONS_PER_PORTION%%", String(m_engine->minRevolutionsPerPortion()));
    motorsettings_html_str.replace("%%MAX_REVOLUTIONS_PER_PORTION%%", String(m_engine->maxRevolutionsPerPortion()));
    motorsettings_html_str.replace("%%REVOLUTIONS_PER_SNACK%%", String(m_engine->revolutionsPerSnack()));
    sendWrappedResponse(request, 200, motorsettings_html_str);
}

void CatServerFrontend::handleFillSensorSettings(AsyncWebServerRequest *request) {

    LOAD_EMBED_FILE(fillsensorsettings_html);
    fillsensorsettings_html_str.replace("%%FULL_MEASUREMENT%%", String(m_fillSensor->fullMeasurement()));
    fillsensorsettings_html_str.replace("%%EMPTY_MEASUREMENT%%", String(m_fillSensor->emptyMeasurement()));
    sendWrappedResponse(request, 200, fillsensorsettings_html_str);
}

void CatServerFrontend::handleFirmware(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_POST) {
        // Handled in handleBody
        return;
    }   

    LOAD_EMBED_FILE(firmware_html);
    sendWrappedResponse(request, 200, firmware_html_str);
}

void CatServerFrontend::handleTimeSettings(AsyncWebServerRequest *request) {
    String timeZoneList;
    for (const TimeSource::TimeZoneEntry &tz : m_timeSource->timeZoneList()) {
        String tzString = String(tz.tzString);
        timeZoneList += "<option value='" + tzString + "'" +
                        (tzString == String(m_timeSource->timeZone().tzString) ? " selected" : "") +
                        ">" + String(tz.friendlyName) + "</option>\n";
    }

    LOAD_EMBED_FILE(timesettings_html);
    timesettings_html_str.replace("%%TIMEZONE_LIST%%", timeZoneList);
    timesettings_html_str.replace("%%CURRENT_DATETIME_STRING%%", m_timeSource->currentDateTimeString());
    timesettings_html_str.replace("%%TIMEZONE_FRIENDLY_NAME%%", m_timeSource->timeZone().friendlyName);
    sendWrappedResponse(request, 200, timesettings_html_str);
}

void CatServerFrontend::handleTimerSettings(AsyncWebServerRequest *request) {
    LOAD_EMBED_FILE(timers_html);
    sendWrappedResponse(request, 200, timers_html_str);
    logInfo(lcWebServer, "Sent /timers response.");
}

void CatServerFrontend::handleDebugLogs(AsyncWebServerRequest *request) {
    LOAD_EMBED_FILE(debuglogs_html);
    sendWrappedResponse(request, 200, debuglogs_html_str);
}

void CatServerFrontend::handleFeed(AsyncWebServerRequest *request) {

    if (!request->hasParam("mode")) {
        request->send(400, "text/plain", "missing mode parameter");
        return;
    }
    
    String mode = request->getParam("mode")->value();
    if (mode == "snack") {
        m_engine->feed(Engine::FeedingMode::Snack);
    } else if (mode == "meal") {
        m_engine->feed();
    } else {
        request->send(400, "text/plain", "invalid mode parameter");
        return;
    }

    if (request->method() == HTTP_POST) {
        request->send(200, "text/html", "OK");
    } else {
        request->redirect("/", 303);
    }
}

void CatServerFrontend::handleReset(AsyncWebServerRequest *request) {
    m_resetFunction();
    LOAD_EMBED_FILE(reset_html);
    sendWrappedResponse(request, 200, reset_html_str);
    m_rebootPending = true;
}

void CatServerFrontend::handleFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static bool uploadStarted = false;

    auto handleResult = [](const String &message) -> String {
        return R"(
            <div class='container'>
                <h2>Firmware Update</h2>
                <div class='info'>
                    <p>)" +
               message + R"(</p>
                </div>
                <form action='/firmware' method='GET'>
                    <button class='btn' style='background:#888;'>Back</button>
                </form>
            </div>
        )";
    };

    if (!m_otaManager) {
        logInfo(lcWebServer, "Firmware upload: no OTAManager set");
        sendWrappedResponse(request, 500, handleResult("No OTAManager configured."));
        return;
    }

    if (index == 0) {
        bool success = m_otaManager->beginUpdate(request->contentLength());
        if (!success) {
            logInfo(lcWebServer, "Failed to start firmware update.");
            sendWrappedResponse(request, 500, handleResult("Failed to start the firmware update."));
            return;
        }
        logInfo(lcWebServer, "Firmware upload started: " + filename + ", size: " + String(request->contentLength()) + " bytes");
        uploadStarted = true;
    }

    if (!uploadStarted) {
        // ignore
        return;
    }

    if (len) {
        logDebug(lcWebServer, "Firmware upload chunk: " + String(len) + " bytes (index " + String(index) + ")");
        if (m_otaManager->write(data, len) != len) {
            logError(lcWebServer, "Firmware update failed to write");
            sendWrappedResponse(request, 500, handleResult("Failed to write the firmware update data."));
        }
    }

    if (final) {
        uploadStarted = false;
        logInfo(lcWebServer, "Firmware upload finished, finishing update...");
        bool success = m_otaManager->endUpdate();
        if (success) {
            sendWrappedResponse(request, 200, handleResult("Flashing completed. Device will reboot now."));
            logInfo(lcWebServer, "Firmware upload succeeded. Rebooting in .5 secs.");
            m_rebootPending = true;
        } else {
            sendWrappedResponse(request, 500, handleResult("Failed to complete firmware update."));
            logError(lcWebServer, "Firmware upload failed.");
        }
    }
}
