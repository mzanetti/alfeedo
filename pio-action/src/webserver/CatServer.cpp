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
#include <SPIFFS.h>

#include "Engine.h"
#include "OTAManager.h"
#include "FillSensor.h"
#include "DisplayController.h"
#include "NetworkConfigManager.h"

#include <ArduinoJson.h>
#include <map>
#include <nvs_flash.h>
#include <pgmspace.h>

CatServer::CatServer(): 
    m_server(80)
{}

CatServer::~CatServer() {}

void CatServer::begin(Engine* engine, FillSensor* fillSensor, OTAManager* otaManager, TimeSource* timeSource, DisplayController* displayController, NetworkConfigManager* networkConfigManager, ResetFunction resetFunction) {
    m_networkConfigManager = networkConfigManager;
    m_networkConfigManager->addMDNSService({"http", 80, {
        {"model", "alfeedo"},
        {"version", CATFEEDER_VERSION},
        {"name", m_networkConfigManager->hostname()},
        {"hwaddr", WiFi.macAddress()}
    }});

    m_server.addHandler(&m_api);
    // Frontend catches all. Keep it last.
    m_server.addHandler(&m_frontend);


    m_server.onNotFound([this](AsyncWebServerRequest *request){
        Serial.println("NOT FOUND: " + request->url());
        handleNotFound(request);
    });

    m_frontend.begin(engine, fillSensor, otaManager, timeSource, displayController, networkConfigManager, resetFunction);
    m_api.begin(engine, fillSensor, otaManager, displayController, networkConfigManager, timeSource);
    m_server.begin();

}

void CatServer::loop()
{
    m_frontend.loop();
}


void CatServer::handleNotFound(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response;

    if (m_networkConfigManager->wifiMode() == NetworkConfigManager::WifiMode::AP) {
        Serial.println("WiFi AP mode. Redirecting to home.");
        response = request->beginResponse(303);
        request->redirect("http://" + WiFi.softAPIP().toString() + "/");
    } else {
        response = request->beginResponse(404, "text/plain", "404: Not Found");
        request->send(response);
    }
}