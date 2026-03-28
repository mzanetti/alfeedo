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

#ifndef CATSERVER_H
#define CATSERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include "webserver/CatServerFrontend.h"
#include "webserver/CatServerApi.h"
#include "ResetHandler.h"

class Engine;
class OTAManager;
class FillSensor;
class TimeSource;
class DisplayController;
class NetworkConfigManager;

class CatServer {
public:
    CatServer();
    ~CatServer();

    void begin(Engine* engine, FillSensor* fillSensor, OTAManager* otaManager, TimeSource* timeSource, DisplayController* displayController, NetworkConfigManager* networkConfigManager, ResetFunction resetFunction);
    void loop();

private:
    AsyncWebServer m_server;

    CatServerFrontend m_frontend;
    CatServerApi m_api;

    NetworkConfigManager* m_networkConfigManager = nullptr;

    void handleNotFound(AsyncWebServerRequest *request);

};

#endif // CATSERVER_H