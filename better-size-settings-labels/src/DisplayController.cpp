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

#include "DisplayController.h"
#include "Engine.h"
#include "FillSensor.h"
#include "Logging.h"

#include "pages/Page.h"
#include "pages/MainPage.h"
#include "pages/FeedPage.h"
#include "pages/SettingsPage.h"
#include "pages/InfoPage.h"
#include "pages/TimersPage.h"

#include <Wire.h>
#include <WiFi.h>

LoggingCategory lcDisplay("DisplayController");

MainPage mainPage;
FeedPage feedPage;
TimersPage timersPage;
SettingsPage settingsPage(&mainPage);
InfoPage infoPage;

std::vector<Page *> s_pages = {
    &mainPage,
    &feedPage,
    &timersPage,
    &settingsPage,
    &infoPage,
};

DisplayController::DisplayController(int i2cAddress, int lcdColumns, int lcdRows) : 
    m_i2cAddress(i2cAddress),
    m_lcdColumns(lcdColumns),
    m_lcdRows(lcdRows)
{
}

DisplayController::~DisplayController() = default;

void DisplayController::setup(Engine *engine, FillSensor *fillSensor, OTAManager *otaManager, TimeSource *timeSource, ResetFunction resetFunction, int feedButtonPin, int nextButtonPin, int previousButtonPin)
{

    m_engine = engine;
    m_fillSensor = fillSensor;
    m_otaManager = otaManager;
    m_timeSource = timeSource;

    m_feedButton.setup(feedButtonPin, [this]() {
        logInfo(lcDisplay, "Feed button pressed");
        m_currentPage->handleInput(this, m_engine, BUTTON_MENU);
        m_lastInteractionTime = time(nullptr);
    });

    m_nextButton.setup(nextButtonPin, [this]() {
        logInfo(lcDisplay, "Next button pressed");
        m_currentPage->handleInput(this, m_engine, BUTTON_NEXT);
        m_lastInteractionTime = time(nullptr);
    });
    m_previousButton.setup(previousButtonPin, [this]() {
        logInfo(lcDisplay, "Previous button pressed");
        m_currentPage->handleInput(this, m_engine, BUTTON_PREVIOUS);
        m_lastInteractionTime = time(nullptr);
    });

    if (!Wire.begin()) {
        logError(lcDisplay, "I2C initialization failed");
        return;
    }

    Wire.beginTransmission(m_i2cAddress);
    if (Wire.endTransmission() != 0) {
        logError(lcDisplay, "I2C device not found at address: " + String(m_i2cAddress));
        return;
    }

    if (!m_display.begin(SSD1306_SWITCHCAPVCC, m_i2cAddress)) {
        logError(lcDisplay, "SSD1306 allocation failed");
        return;
    }
    logInfo(lcDisplay, "SSD1306 initialized");

    m_display.display();
    m_display.setTextColor(WHITE);

    settingsPage.setTimeSource(timeSource);
    settingsPage.setResetFunction(resetFunction);
    mainPage.begin();

    setCurrentPage(&mainPage);
}

void DisplayController::setCurrentPage(Page *page)
{
    logInfo(lcDisplay, "Setting current page: " + page->name());
    m_currentPage = page;
    m_currentPage->reset();
    m_display.dim(false);
    refresh();
    m_lastInteractionTime = time(nullptr);
}

void DisplayController::loop()
{
    if (m_currentPage) {

        m_feedButton.loop();
        m_previousButton.loop();
        m_nextButton.loop();

        time_t now = time(nullptr);
        if (m_currentPage->autoExit() && now - m_lastInteractionTime > 5) {
            time_t now = time(nullptr);
            if (now - m_lastInteractionTime > 5) {
                setCurrentPage(&mainPage);
                m_display.dim(true);
            }
        }
        refresh();
    }
}

void DisplayController::refresh()
{
    if (m_currentPage && m_engine && m_fillSensor && m_otaManager) {
        m_currentPage->render(m_display, m_engine, m_fillSensor, m_otaManager);
    }
}

void DisplayController::nextMenuPage()
{
    logInfo(lcDisplay, "Next menu page");
    setCurrentPage(s_pages[(std::distance(s_pages.begin(), std::find(s_pages.begin(), s_pages.end(), m_currentPage)) + 1) % s_pages.size()]);
}

void DisplayController::previousMenuPage()
{
    logInfo(lcDisplay, "Previous menu page");
    setCurrentPage(s_pages[(std::distance(s_pages.begin(), std::find(s_pages.begin(), s_pages.end(), m_currentPage)) - 1 + s_pages.size()) % s_pages.size()]);
}

void DisplayController::exitMenu()
{
    logInfo(lcDisplay, "Exiting menu to main page");
    setCurrentPage(&mainPage);
}   

void DisplayController::reset()
{
    logInfo(lcDisplay, "Resetting display controller...");
    mainPage.setHotKeyFunction(MainPage::HotKeyFunctionFeed);
}