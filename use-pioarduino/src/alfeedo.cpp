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

#include "webserver/CatServer.h"
#include "DisplayController.h"
#include "Engine.h"
#include "FillSensor.h"
#include "Motor.h"
#include "OTAManager.h"
#include "TimeSource.h"
#include "NetworkConfigManager.h"
#include "hwsettings.h"
#include "Logging.h"

#include <Arduino.h>
#include <SPIFFS.h>

#define DISPLAY_I2C_ADDR 0x3C
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 32

LoggingCategory lcMain("Main");

Engine engine;
CatServer catserver;
Motor motor(DRIVER_STEP_PIN, DRIVER_DIRECTION_PIN, DRIVER_ENABLE_PIN, Serial2, UART_RX_PIN, UART_TX_PIN, DRIVER_DIAG_PIN);
OTAManager otaManager;
DisplayController displayController(DISPLAY_I2C_ADDR, DISPLAY_WIDTH, DISPLAY_HEIGHT);
FillSensor fillSensor;
TimeSource timeSource;
NetworkConfigManager networkConfigManager;

namespace ResetHandler {
    void resetAll() {
        logInfo(lcMain, "Performing factory reset...");
        engine.reset();
        displayController.reset();
        fillSensor.reset();
        timeSource.reset();
        networkConfigManager.reset();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    logInfo(lcMain, "Meeeow!");

    if (!SPIFFS.begin(true)) {
        logError(lcMain, "Unable to mount SPIFFS");
        return;
    }

    timeSource.begin();
    
    networkConfigManager.begin();

    fillSensor.begin();
    
    engine.begin(&timeSource, &motor, &fillSensor);
    
    displayController.setup(&engine, &fillSensor, &otaManager, &timeSource, ResetHandler::resetAll, FEED_BUTTON_PIN, NEXT_BUTTON_PIN, PREVIOUS_BUTTON_PIN);

    catserver.begin(&engine, &fillSensor, &otaManager, &timeSource, &displayController, &networkConfigManager, ResetHandler::resetAll);
}

void loop() {
    timeSource.loop();
    engine.update();
    motor.loop();
    displayController.loop();
    fillSensor.loop();
    catserver.loop();
    networkConfigManager.loop();
}
