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

#include <ArduinoJson.h>
#include "CatServerApi.h"

#include "Engine.h"
#include "OTAManager.h"
#include "FillSensor.h"
#include "DisplayController.h"
#include "NetworkConfigManager.h"
#include "Logging.h"
#include "TimeSource.h"

#include <map>
#include <time.h>

LoggingCategory lcApi("Api");

void CatServerApi::begin(Engine *engine, FillSensor *fillSensor, OTAManager *otaManager, DisplayController *displayController, NetworkConfigManager *networkConfigManager, TimeSource *timeSource) {
    m_engine = engine;
    m_otaManager = otaManager;
    m_fillSensor = fillSensor;
    m_displayController = displayController;
    m_networkConfigManager = networkConfigManager;
    m_timeSource = timeSource;
}

bool CatServerApi::canHandle(AsyncWebServerRequest *request) const {
        return request->url().startsWith("/api/");
}

void CatServerApi::handleRequest(AsyncWebServerRequest *request) {
    String fullPath = request->url();

    int apiIndex = fullPath.indexOf("/api/");
    String endpoint = fullPath.substring(apiIndex + 4);

#if DEBUG_WEBSERVER_API
    String argList = "";
    for (uint8_t i = 0; i < request->args(); i++) {
        argList += request->argName(i) + "=" + request->arg(i) + "&";
    }
    logDebug(lcApi, "Handling request for path: " + fullPath + " Method: " + String((int)request->method()) + " Args: " + String(request->args()) + " [" + argList + "]");
#endif

    std::map<String, void (CatServerApi::*)(AsyncWebServerRequest *)> routeHandlers = {
        {"/status", &CatServerApi::handleStatus},
        {"/fillsensor", &CatServerApi::handleFillSensor},
        {"/timers", &CatServerApi::handleTimers},
        {"/logs", &CatServerApi::handleLogs},
        {"/feed", &CatServerApi::handleFeed},
        {"/debuglogs", &CatServerApi::handleDebugLogs},
        {"/settings/motor", &CatServerApi::handleMotorSettings},
        {"/settings/fillsensor", &CatServerApi::handleFillSensorSettings},
        {"/settings/time", &CatServerApi::handleTimeSettings}
    };

    auto it = routeHandlers.find(endpoint);;
    if (it != routeHandlers.end()) {
        (this->*(it->second))(request);
        return;
    }
    
    logWarning(lcApi, "No handler found for path: " + fullPath);
    request->send(404, "application/json", "{\"status\":\"error\",\"error\":\"API Endpoint Not Found\"}");
}

void CatServerApi::handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String fullPath = request->url();

    int apiIndex = fullPath.indexOf("/api/");
    String endpoint = fullPath.substring(apiIndex + 4);

#if DEBUG_WEBSERVER_API
    String argList = "";
    for (uint8_t i = 0; i < request->args(); i++) {
        argList += request->argName(i) + "=" + request->arg(i) + "&";
    }
    logInfo(lcApi, "Handling body request for path: " + fullPath + " Method: " + String((int)request->method()) + " Args: " + String(request->args()) + " [" + argList + "]");
#endif

    std::map<String, void (CatServerApi::*)(AsyncWebServerRequest *, uint8_t *, size_t, size_t, size_t)> routeHandlers = {
        {"/feed", &CatServerApi::handleFeedBody},
        {"/timers", &CatServerApi::handleTimersBody},
        {"/settings/motor", &CatServerApi::handleMotorSettingsBody},
        {"/settings/fillsensor", &CatServerApi::handleFillSensorSettingsBody},
        {"/settings/time", &CatServerApi::handleTimeSettingsBody}
    };

    auto it = routeHandlers.find(endpoint);;
    if (it != routeHandlers.end()) {
        (this->*(it->second))(request, data, len, index, total);
        return;
    }

    logWarning(lcApi, "No body handler found for path: " + fullPath);

    request->send(404, "application/json", "{\"status\":\"error\",\"error\":\"API Endpoint Not Found\"}");
}

void CatServerApi::handleStatus(AsyncWebServerRequest *request) {
    std::map<Engine::State, String> stateMap = {
        {Engine::State::Idle, "idle"},
        {Engine::State::Moving, "moving"}
    };
    std::map<Engine::ErrorState, String> errorStateMap = {
        {Engine::ErrorState::NoError, "ok"},
        {Engine::ErrorState::Struggling, "struggling"},
        {Engine::ErrorState::Jammed, "jammed"}
    };
    JsonDocument doc;
    doc["name"] = m_networkConfigManager->hostname();
    doc["time"] = m_timeSource->currentDateTimeString();
    doc["timezone"] = m_timeSource->timeZone().friendlyName;
    doc["state"] =  stateMap.at(m_engine->state());
    doc["fillLevel"] = m_fillSensor ? m_fillSensor->fillLevel() : -1;
    doc["errorState"] = errorStateMap.at(m_engine->errorState());
    doc["lastStallCount"] = m_engine->lastStallCount();
    JsonObject nextTimerJson = doc["nextTimer"].to<JsonObject>();    
    nextTimerJson["time"] = m_engine->nextTimer().time();
    nextTimerJson["mode"] = TimerEntry::timerModeToString(m_engine->nextTimer().mode());

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void CatServerApi::handleFillSensor(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["measurement"] = m_fillSensor->measurement();
    doc["fillLevel"] = m_fillSensor->fillLevel();
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void CatServerApi::handleTimers(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_POST || request->method() == HTTP_PUT) {
        logDebug(lcApi, "handleTimer API POST/PUT request received");
        // processed in handleTimersBody, just return here
        return; 
    }

    if (request->method() == HTTP_DELETE) {
        logDebug(lcApi, "handleTimer API DELETE request received");
        if (!request->hasArg("timer_id")) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Missing timer_id parameter\"}");
            return;
        }
        int timerId = request->arg("timer_id").toInt();
        if (!m_engine->removeTimer(timerId)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid timer_id parameter\"}");
            return;
        }
        
        request->send(200, "application/json", "{\"status\":\"success\"}");
        return;
    }

    JsonDocument doc;
    doc["maxTimers"] = Engine::MaxTimers;
    JsonArray timers = doc["timers"].to<JsonArray>();
    for (int i = 0; i < m_engine->timers().size(); i++) {
        const TimerEntry &entry = m_engine->timers()[i];
        JsonObject timerJson = timers.add<JsonObject>();
        timerJson["id"] = i;
        timerJson["time"] = entry.time();
        timerJson["mode"] = TimerEntry::timerModeToString(entry.mode());
    }
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void CatServerApi::handleTimersBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    logDebug(lcApi, "Handling timer body data: " + String(len) + " bytes, index: " + String(index) + ", total: " + String(total));

    if (index + len < total) {
        logError(lcApi, "Received incomplete timer data: " + String(index + len) + " bytes received, expected total: " + String(total));
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Incomplete data received\"}");
        return; 
    }

    if (request->method() == HTTP_POST) {
        logInfo(lcApi, "handleTimersBody POST body received. Adding new timer.");
        JsonDocument doc; 
        DeserializationError error = deserializeJson(doc, data, len);

        if (error) {
            logWarning(lcApi, "JSON Deserialization failed");
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid JSON\"}");
            return;
        }

        const char* timeString = doc["time_h_m"];
        const char* mode = doc["mode"];

        if (!timeString || !mode) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Missing fields\"}");
            return;
        }

        String ts(timeString);
        int hours = ts.substring(0, 2).toInt();
        int minutes = ts.substring(3, 5).toInt();
        
        TimerEntry entry((hours * 60) + minutes, TimerEntry::timerModeFromString(mode));
        if (!m_engine->addTimer(entry)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Failed to add timer. Maximum number of timers reached.\"}");
            return;
        }

        request->send(200, "application/json", "{\"status\":\"success\"}");
    }

    if (request->method() == HTTP_PUT) {
        logInfo(lcApi, "handleTimersBody PUT body received. Updating timer.");
        JsonDocument doc; 
        DeserializationError error = deserializeJson(doc, data, len);

        if (error) {
            logWarning(lcApi, "JSON Deserialization failed");
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid JSON\"}");
            return;
        }

        const char* timerIdStr = doc["timer_id"];
        const char* timeString = doc["time_h_m"];
        const char* mode = doc["mode"];

        if (!timerIdStr || !timeString || !mode) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Missing fields\"}");
            return;
        }

        int id = atoi(timerIdStr);
        String ts(timeString);
        int hours = ts.substring(0, 2).toInt();
        int minutes = ts.substring(3, 5).toInt();
        
        TimerEntry entry((hours * 60) + minutes, TimerEntry::timerModeFromString(mode));
        if (!m_engine->updateTimer(id, entry)) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Failed to update timer.\"}");
            return;
        }

        logInfo(lcApi, "Timer updated");
        request->send(200, "application/json", "{\"status\":\"success\"}");
    }

}

void CatServerApi::handleLogs(AsyncWebServerRequest *request) {
    logDebug(lcApi, "Handling /logs request");

    std::vector<Engine::LogEntry> logs = m_engine->logEntries();
    
    JsonDocument doc;
    JsonArray logArray = doc["logs"].to<JsonArray>();

    for (const auto& logEntry : logs) {
        JsonObject logObject = logArray.add<JsonObject>();
        logObject["timestamp"] = logEntry.timestamp;
        
        // Use safer mapping to prevent crashes on unknown types
        if (logEntry.type == Engine::LogEntry::TypeManualFeed) logObject["type"] = "manual";
        else if (logEntry.type == Engine::LogEntry::TypeTimerFeed) logObject["type"] = "timer";
        else logObject["type"] = "unknown";

        if (logEntry.mode == Engine::FeedingMode::Meal) logObject["mode"] = "meal";
        else if (logEntry.mode == Engine::FeedingMode::Snack) logObject["mode"] = "snack";
        else logObject["mode"] = "unknown";

        logObject["stallCount"] = logEntry.stallCount;
        logObject["aborted"] = logEntry.aborted;
        logObject["fillLevel"] = logEntry.fillLevel;
    }
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void CatServerApi::handleDebugLogs(AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray logs = doc["logs"].to<JsonArray>();
    for (const LogMessage &msg : Logging::logs()) {
        JsonObject logObject = logs.add<JsonObject>();
        logObject["timestamp"] = msg.timestamp();
        logObject["category"] = msg.category().category();
        logObject["level"] = static_cast<int>(msg.level());
        logObject["message"] = msg.message();
    }
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void CatServerApi::handleFeedBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    
    JsonDocument input;
    DeserializationError error = deserializeJson(input, data, len);
        
    if (error) {
        logWarning(lcApi, "deserializeJson() failed: " + String(error.c_str()));
        return;
    }

    request->_tempObject = new String(input["mode"].as<String>());
}

void CatServerApi::handleFeed(AsyncWebServerRequest *request) {

    if (request->method() != HTTP_POST) {
        request->send(404, "application/json", "{\"status\":\"error\",\"error\":\"API Endpoint Not Found\"}");
        return;
    }

    int status = 200;
    String errorMessage;

    if (request->_tempObject == nullptr) {
        logWarning(lcApi, "Feed mode not found in request");
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"missing feed mode\"}");
        return;
    }
    String *mode = static_cast<String*>(request->_tempObject);
    request->_tempObject = nullptr;

    if (*mode == "meal") {
        m_engine->feed();
    } else if (*mode == "snack") {
        m_engine->feed(Engine::FeedingMode::Snack);
    } else {
        status = 400;
        errorMessage = "Unknown feed mode: " + *mode;
    }
     
    delete mode;

    JsonDocument doc;
    doc["status"] = status;
    doc["errorMessage"] = errorMessage;
    String output;
    serializeJson(doc, output);
    logDebug(lcApi, "API POST response: " + String(status) + ": " + output);
    request->send(status, "application/json", output);

}

void CatServerApi::handleMotorSettings(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_POST) {
        return;
    }

    if (request->method() == HTTP_GET) {
        JsonDocument doc;
        doc["minSpeed"] = m_engine->minSpeed();
        doc["maxSpeed"] = m_engine->maxSpeed();
        doc["speed"] = m_engine->speed();
        doc["minRevolutionsPerPortion"] = m_engine->minRevolutionsPerPortion();
        doc["maxRevolutionsPerPortion"] = m_engine->maxRevolutionsPerPortion();
        doc["revolutionsPerPortion"] = m_engine->revolutionsPerPortion();
        doc["revolutionsPerSnack"] = m_engine->revolutionsPerSnack();
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    request->send(404, "application/json", "{\"status\":\"error\",\"error\":\"API Endpoint Not Found\"}");
}

void CatServerApi::handleMotorSettingsBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    JsonDocument input;
    DeserializationError error = deserializeJson(input, data, len);
        
    if (error) {
        logWarning(lcApi, "deserializeJson() failed: " + String(error.c_str()));
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid JSON\"}");
        return;
    }

    JsonVariant speedVariant = input["speed"];
    JsonVariant revolutionsPerPortionVariant = input["revolutionsPerPortion"];
    JsonVariant revolutionsPerSnackVariant = input["revolutionsPerSnack"];
    float speed = speedVariant.as<float>();
    float revolutionsPerPortion = revolutionsPerPortionVariant.as<float>();
    float revolutionsPerSnack = revolutionsPerSnackVariant.as<float>();

    if (!speedVariant.isNull() && (speed < m_engine->minSpeed() || speed > m_engine->maxSpeed())) {
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Speed value out of range\"}");
        return;
    }

    if (!revolutionsPerPortionVariant.isNull() && (revolutionsPerPortion < m_engine->minRevolutionsPerPortion() || revolutionsPerPortion > m_engine->maxRevolutionsPerPortion())) {
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Revolutions per portion value out of range\"}");
        return;
    }

    if (!revolutionsPerSnackVariant.isNull() && (revolutionsPerSnack < m_engine->minRevolutionsPerPortion() || revolutionsPerSnack > m_engine->maxRevolutionsPerPortion())) {
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Revolutions per snack value out of range\"}");
        return;
    }

    if (!speedVariant.isNull()) {
        m_engine->setSpeed(speed);
    }
    if (!revolutionsPerPortionVariant.isNull()) {
        m_engine->setRevolutionsPerPortion(revolutionsPerPortion);
    }
    if (!revolutionsPerSnackVariant.isNull()) {
        m_engine->setRevolutionsPerSnack(revolutionsPerSnack);
    }

    request->send(200, "application/json", "{\"status\":\"success\"}");
}   

void CatServerApi::handleFillSensorSettings(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_POST) {
        return;
    }

    if (request->method() == HTTP_GET) {
        JsonDocument doc;
        doc["emptyMeasurement"] = m_fillSensor->emptyMeasurement();
        doc["fullMeasurement"] = m_fillSensor->fullMeasurement();
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    request->send(404, "application/json", "{\"status\":\"error\",\"error\":\"API Endpoint Not Found\"}");
}

void CatServerApi::handleFillSensorSettingsBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    JsonDocument input;
    DeserializationError error = deserializeJson(input, data, len);
        
    if (error) {
        logWarning(lcApi, "deserializeJson() failed: " + String(error.c_str()));
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid JSON\"}");
        return;
    }

    JsonVariant emptyMeasurementVariant = input["emptyMeasurement"];
    JsonVariant fullMeasurementVariant = input["fullMeasurement"];
    int emptyMeasurement = emptyMeasurementVariant.as<int>();
    int fullMeasurement = fullMeasurementVariant.as<int>();

    if (emptyMeasurementVariant.isNull() || fullMeasurementVariant.isNull()) {
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Missing parameters\"}");
        return;
    }

    if ((fullMeasurement >= emptyMeasurement)) {
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Full measurement value must be smaller than empty measurement value\"}");
        return;
    }

    m_fillSensor->setEmptyMeasurement(emptyMeasurement);
    m_fillSensor->setFullMeasurement(fullMeasurement);

    request->send(200, "application/json", "{\"status\":\"success\"}");
}   

void CatServerApi::handleTimeSettings(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_POST) {
        return;
    }

    if (request->method() == HTTP_GET) {
        JsonDocument doc;
        doc["currentDateTime"] = m_timeSource->currentDateTimeString();
        doc["tzString"] = m_timeSource->timeZone().tzString;
        doc["timezone"] = m_timeSource->timeZone().friendlyName;
        doc["ntpSynchronized"] = m_timeSource->ntpSynchronized();
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
        return;
    }

    request->send(404, "application/json", "{\"status\":\"error\",\"error\":\"API Endpoint Not Found\"}");
}

void CatServerApi::handleTimeSettingsBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    JsonDocument input;
    DeserializationError error = deserializeJson(input, data, len);
        
    if (error) {
        logWarning(lcApi, "deserializeJson() failed: " + String(error.c_str()));
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid JSON\"}");
        return;
    }

    JsonVariant timezone = input["timezone"];
    if (!timezone.isNull()) {
        if (!m_timeSource->setTimeZone(timezone.as<String>())) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid timezone value\"}");
            return;
        }   
    }

    JsonVariant datetime = input["datetime"];
    if (!datetime.isNull()) {
        String datetimeString = datetime.as<String>();
        struct tm tm = {0};
        if (strptime(datetimeString.c_str(), "%Y-%m-%dT%H:%M:%S", &tm) == nullptr) {
            request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Invalid datetime format. Expected format: YYYY-MM-DDTHH:MM\"}");
            return;
        }
        tm.tm_isdst = -1;
        m_timeSource->setCurrentDateTime(tm);
    }

    request->send(200, "application/json", "{\"status\":\"success\"}");
}