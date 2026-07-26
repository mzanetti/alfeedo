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

#ifndef DISPLAY_CONTROLLER_H
#define DISPLAY_CONTROLLER_H

#include <Arduino.h>

#include <SPI.h>
// #include "U8g2lib.h"
#include <Adafruit_SSD1306.h>
#include "Wire.h"
#include "Input.h"
#include "ResetHandler.h"

#include <vector>

class Engine;
class FillSensor;
class OTAManager;
class Page;
class TimeSource;

class DisplayController {
public:
    enum ButtonInput
    {
        BUTTON_MENU,
        BUTTON_NEXT,
        BUTTON_PREVIOUS
    };

    DisplayController(int i2cAddress = 0x3C, int lcdColumns = 128, int lcdRows = 32);
    ~DisplayController();


    void setup(Engine *engine, FillSensor *fillSensor, OTAManager *otaManager, TimeSource *timeSource, ResetFunction resetFunction, int feedButtonPin, int nextButtonPin, int previousButtonPin);
    void setCurrentPage(Page *page);
    void refresh();
    void reset();

    void loop();

private:
    friend class Page;
    void nextMenuPage();
    void previousMenuPage();
    void exitMenu();

    // U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C m_display;
    Adafruit_SSD1306 m_display;
    int m_i2cAddress = 0;
    int m_lcdColumns = 0;
    int m_lcdRows = 0;

    Input m_feedButton;
    Input m_nextButton;
    Input m_previousButton;

    Engine *m_engine = nullptr;
    FillSensor *m_fillSensor = nullptr;
    OTAManager *m_otaManager = nullptr;
    TimeSource *m_timeSource = nullptr;

    Page *m_currentPage = nullptr;
    time_t m_lastInteractionTime = 0;
};

#endif // DISPLAY_CONTROLLER_H