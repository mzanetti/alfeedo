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

#ifndef CATSERVERFRONTEND_H
#define CATSERVERFRONTEND_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <map>

#include "ResetHandler.h"
#include "NetworkConfigManager.h"

class Engine;
class OTAManager;
class FillSensor;
class TimeSource;
class DisplayController;

class CatServerFrontend: public AsyncWebHandler {
public:
    CatServerFrontend();
    void begin(Engine* engine, FillSensor* fillSensor, OTAManager* otaManager, TimeSource* timeSource, DisplayController* displayController, NetworkConfigManager* networkConfigManager, ResetFunction resetFunction);
    void loop();

    bool canHandle(AsyncWebServerRequest* request) const override;
    void handleRequest(AsyncWebServerRequest * request) override;
    void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) override;
    void handleUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) override;
    bool isRequestHandlerTrivial() const override {
        return false;
    }

private:
    void handleRoot(AsyncWebServerRequest *request);
    void handleLogs(AsyncWebServerRequest *request);
    void handleSettings(AsyncWebServerRequest *request);
    void handleTimeSettings(AsyncWebServerRequest *request);
    void handleTimerSettings(AsyncWebServerRequest *request);
    void handleNetworkSettings(AsyncWebServerRequest *request);
    void handleNetworkChanged(AsyncWebServerRequest *request);
    void handleMotorSettings(AsyncWebServerRequest *request);
    void handleFillSensorSettings(AsyncWebServerRequest *request);
    void handleFirmware(AsyncWebServerRequest *request);
    void handleFeed(AsyncWebServerRequest *request);
    void handleReset(AsyncWebServerRequest *request);
    void handleDebugLogs(AsyncWebServerRequest *request);
    
    void handleFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

    void sendWrappedResponse(AsyncWebServerRequest *request, int code, const String &body);

    Engine* m_engine = nullptr;
    OTAManager* m_otaManager = nullptr;
    FillSensor* m_fillSensor = nullptr;
    TimeSource* m_timeSource = nullptr;
    DisplayController* m_displayController = nullptr;
    NetworkConfigManager* m_networkConfigManager = nullptr;

    ResetFunction m_resetFunction;

    bool m_rebootPending = false;
    enum class WiFiConfigState {
        Idle,
        Confirming,
        Pending
    };
    WiFiConfigState m_wifiConfigState = WiFiConfigState::Idle;
    NetworkConfigManager::WifiMode m_pendingWiFiMode;
    String m_pendingSSID;
    String m_pendingPassword;

    std::map<String, void (CatServerFrontend::*)(AsyncWebServerRequest *)> m_routeHandlers;
};

#endif
