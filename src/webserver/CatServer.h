/*
This file is part of alfeedo.
...
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
