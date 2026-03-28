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

#ifndef CATSERVERAPI_H
#define CATSERVERAPI_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class Engine;
class OTAManager;
class FillSensor;
class DisplayController;
class NetworkConfigManager;
class TimeSource;

class CatServerApi: public AsyncWebHandler {
public:
    void begin(Engine* engine, FillSensor* fillSensor, OTAManager* otaManager, DisplayController* displayController, NetworkConfigManager* networkConfigManager, TimeSource* timeSource);

    bool canHandle(AsyncWebServerRequest* request) const override;
    void handleRequest(AsyncWebServerRequest* request) override;
    void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override;
    bool isRequestHandlerTrivial() const override {
        return false;
    }
private:
    void handleStatus(AsyncWebServerRequest *request);
    void handleFillSensor(AsyncWebServerRequest *request);
    void handleTimers(AsyncWebServerRequest *request);
    void handleLogs(AsyncWebServerRequest *request);
    void handleFeed(AsyncWebServerRequest *request);
    void handleDebugLogs(AsyncWebServerRequest *request);
    void handleMotorSettings(AsyncWebServerRequest *request);
    void handleFillSensorSettings(AsyncWebServerRequest *request);
    void handleTimeSettings(AsyncWebServerRequest *request);

    void handleFeedBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleTimersBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleMotorSettingsBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleFillSensorSettingsBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleTimeSettingsBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

    Engine* m_engine = nullptr;
    OTAManager* m_otaManager = nullptr;
    FillSensor* m_fillSensor = nullptr;
    DisplayController* m_displayController = nullptr;
    NetworkConfigManager* m_networkConfigManager = nullptr;
    TimeSource* m_timeSource = nullptr;
};

#endif // CATSERVERAPI_H