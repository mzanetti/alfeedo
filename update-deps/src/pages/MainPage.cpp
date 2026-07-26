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

#include "pages/MainPage.h"

#include "FillSensor.h"
#include "Engine.h"
#include "OTAManager.h"

#include <WiFi.h>

MainPage::MainPage(): Page("MainPage")
{
}

void MainPage::begin()
{
    m_prefs.begin("catfeeder", true); // read-only
    m_hotKeyFunction = (HotKeyFunction)m_prefs.getInt("hotKeyFunction", HotKeyFunctionFeed);
    m_prefs.end();
}

void MainPage::render(Adafruit_SSD1306 &display, Engine *engine, FillSensor *fillSensor, OTAManager *otaManager)
{
    bool needsRefresh = false;
    static int oldWifiState = WL_NO_SHIELD;
    if (oldWifiState != WiFi.status()) {
        oldWifiState = WiFi.status();
        needsRefresh = true;
    }

    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Refreshing will cause stutter, not refreshing time while moving
    static unsigned long lastUpdate = 0;
    if ((now < lastUpdate || now - lastUpdate >= 1) && engine->state() == Engine::State::Idle) {
        lastUpdate = now;
        needsRefresh = true;
    }
    static Engine::State oldState = Engine::State::Idle;
    if (oldState != engine->state()) {
        oldState = engine->state();
        needsRefresh = true;
    }
    static int oldFillLevel = -1;
    int currentFillLevel = fillSensor->fillLevel();
    if (oldFillLevel != currentFillLevel) {
        oldFillLevel = currentFillLevel;
        needsRefresh = true;
    }
    static int oldOtaProgress = 0;
    static bool oldOtaInProgress = false;
    if (oldOtaInProgress != otaManager->updateInProgress() || oldOtaProgress != otaManager->updateProgress()) {
        oldOtaInProgress = otaManager->updateInProgress();
        oldOtaProgress = otaManager->updateProgress();
        needsRefresh = true;
    }

    if (!needsRefresh) {
        return;
    }

    display.clearDisplay();

    if (otaManager->updateInProgress()) {
        drawCenteredText(display, 0, "Updating: " + String(otaManager->updateProgress()) + " %", 1, "", 1);
        display.drawRect(0, display.height() - 10, display.width(), 5, WHITE);
        display.drawRect(0, display.height() - 9, map(otaManager->updateProgress(), 0, 100, 0, display.width()), 3, WHITE);
        display.display();
        return;
    }

    bool blinkOn = now % 2 == 0;

    if (WiFi.status() == WL_CONNECTED) {
        display.drawXBitmap(display.width() - ICON_WIDTH, 0, epd_bitmap_wifi_full, ICON_WIDTH, ICON_HEIGHT, SSD1306_WHITE);
    }
    bool timerSet = false;
    for (const TimerEntry &entry : engine->timers()) {
        if (entry.mode() != TimerEntry::TimerMode::Off) {
            timerSet = true;
            break;
        }
    }

    String fillText = String(fillSensor->fillLevel()) + "%";
    display.drawRect(2, 0, 12, 16, WHITE);
    uint8_t offset = map(fillSensor->fillLevel(), 0, 100, 16, 0);
    display.fillRect(2, offset, 12, 16 - offset, WHITE);

    display.setTextSize(1);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(fillText, 0, 0, &x1, &y1, &w, &h);
    int16_t textX = max(0, (16 - w) / 2);
    display.setCursor(textX, 16 + (16 - h) / 2);
    display.print(fillText);

    if (timerSet) {
        display.drawXBitmap(display.width() - ICON_WIDTH, 16, epd_bitmap_alarm, ICON_WIDTH, ICON_HEIGHT, WHITE);
    }

    
    char timeStr[32];
    if (blinkOn) {
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    } else {
        snprintf(timeStr, sizeof(timeStr), "%02d %02d", timeinfo.tm_hour, timeinfo.tm_min);
    }
    String timeString = timeStr;

    if (fillSensor->fillLevel() < 10 && !blinkOn && engine->state() == Engine::State::Idle) {
        display.drawXBitmap((display.width() - 24) / 2, (display.height() - 24) / 2, epd_bitmap_warning, 24, 24, SSD1306_WHITE);
        display.display();
        display.dim(false);
        return;
    }

    String subText = m_hotKeyFunction == HotKeyFunctionOff ? "" : hotkeys().at(m_hotKeyFunction);
    drawCenteredText(display, 0, engine->state() == Engine::State::Moving ? "Feeding" : timeString, 2, subText, 1);

    display.dim(true);

    display.display();
}

void MainPage::handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input)
{
    switch (input)
    {
    case DisplayController::BUTTON_MENU:
        switch (m_hotKeyFunction) {
        case HotKeyFunctionFeed:
            engine->feed();
            break;
        case HotKeyFunctionSnack:
            engine->feed(Engine::FeedingMode::Snack);
            break;
        case HotKeyFunctionOff:
            Serial.println("HotKey disabled.");
            break;
        }
    default:
        Page::handleInput(controller, engine, input);
        break;
    }
}

MainPage::HotKeyFunction MainPage::hotKeyFunction() const
{
    return m_hotKeyFunction;
}

void MainPage::setHotKeyFunction(MainPage::HotKeyFunction hotKeyFunction)
{
    m_hotKeyFunction = hotKeyFunction;
    m_prefs.begin("catfeeder");
    m_prefs.putInt("hotKeyFunction", (int)m_hotKeyFunction);
    Serial.println("Storing hotkey: " + String((int)m_hotKeyFunction));
    m_prefs.end();
}

