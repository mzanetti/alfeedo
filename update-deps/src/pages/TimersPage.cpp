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

#include "TimersPage.h"

#include "Engine.h"

#include <map>

#define MINUTES_PER_DAY (24 * 60)

TimersPage::TimersPage():
    Page("TimersPage")
{
}

void TimersPage::render(Adafruit_SSD1306 &display, Engine *engine, FillSensor *FillSensor, OTAManager *otaManager)
{
    static bool oldBlinkOn = true;
    bool blinkOn = (millis() / 500) % 2 == 0;
    if (oldBlinkOn != blinkOn) {
        oldBlinkOn = blinkOn;
        m_needsRefresh = true;
    }

    if (!m_needsRefresh) {
        return;
    }
    m_needsRefresh = false;

    display.clearDisplay();

    if (!m_active) {
        drawCenteredText(display, m_cycle, "Timer", 2, "", 1);
        display.display();
        return;
    }

    
    if (m_selection >= engine->timers().size()) {
        drawMenu(display, m_cycle, 0, "New timer", 2);
        // drawCenteredText(display, m_cycle,"< " + timerName + " >", 1, "New timer", 2);
        display.display();
        return;
    }
    String timerId = String(m_selection);

    TimerEntry entry = engine->timers().at(m_selection);
    char timeString[6];
    int8_t hours = floor(entry.time() / 60);
    int8_t minutes = entry.time() % 60;
    sprintf(timeString, "%02d:%02d", hours, minutes);

    String modeString = TimerEntry::timerModeToString(entry.mode());
        
    if (!m_editMode) {
        drawMenu(display, m_cycle, 0, timerId + ": " + modeString, 2, entry.timeString(), 1);
        display.display();
        return;
    }

    uint8_t line1Size = m_editField == EditFieldMode ? 2 : 1;
    uint8_t line2Size = m_editField == EditFieldMode ? 1 : 2;
    if (!blinkOn) {
        switch (m_editField) {
        case EditFieldMode:
            modeString = "     ";
            break;
        case EditFieldHour:
            sprintf(timeString, "  :%02d", minutes);
            break;
        case EditFieldMinute:
            sprintf(timeString, "%02d:  ", hours);
            break;
        }
    }    
    drawMenu(display, m_cycle, -1, timerId + ": " + modeString, line1Size, timeString, line2Size);

    // if (blinkOn) {
    //     int16_t x1, y1;
    //     uint16_t w, h;
    //     display.setTextSize(2);
    //     display.getTextBounds(timeString, 0, 0, &x1, &y1, &w, &h);
    //     int bottomX = (display.width() - w) / 2;
    //     int bottomY = (8 + 5) * 2;

    //     uint16_t fieldWidth;
    //     display.getTextBounds("00", 0, 0, &x1, &y1, &fieldWidth, &h);

    //     if (m_editField > 0) {
    //         uint16_t secondFieldOffset;
    //         display.getTextBounds("00:", 0, 0, &x1, &y1, &secondFieldOffset, &h);
    //         bottomX += secondFieldOffset;
    //     }

    //     display.drawLine(bottomX, bottomY + h + 1, bottomX + fieldWidth, bottomY + h +1, WHITE);
    // }

    display.display();
}

void TimersPage::handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input)
{
    m_needsRefresh = true;

    if (!m_active) {
        if (input == DisplayController::ButtonInput::BUTTON_MENU) {
            m_active = true;
        } else {
            Page::handleInput(controller, engine, input);
        }
        return;
    }

    if (!m_editMode) {
        switch (input) {
        case DisplayController::ButtonInput::BUTTON_MENU:
            m_editMode = true;
            if (m_selection >= engine->timers().size()) {
                Serial.println("Creating new timer");
                engine->addTimer(TimerEntry());
            }
            break;
        case DisplayController::ButtonInput::BUTTON_NEXT:
            m_selection = (m_selection + 1) % (engine->timers().size() + 1);
            break;
        case DisplayController::ButtonInput::BUTTON_PREVIOUS:
            m_selection = (m_selection - 1 + engine->timers().size() + 1) % (engine->timers().size() + 1);
            break;
        }
        return;
    }

    std::vector<TimerEntry> timers = engine->timers();
    Serial.println("Have timers: " + String(timers.size()));
    TimerEntry entry = timers.at(m_selection);

    std::vector<TimerEntry::TimerMode> modes = {
        TimerEntry::TimerMode::Off,
        TimerEntry::TimerMode::Meal,
        TimerEntry::TimerMode::Snack
    };
    switch (input) {
    case DisplayController::ButtonInput::BUTTON_MENU:
        if (m_editField == EditFieldMode) {
            m_editField = EditFieldHour;
        } else if (m_editField == EditFieldHour) {
            m_editField = EditFieldMinute;
        } else {
            m_editField = EditFieldMode;
            m_editMode = false;
        }
        break;
    case DisplayController::ButtonInput::BUTTON_NEXT:
        if (m_editField == EditFieldMode) {
            entry.setMode(modes[(std::distance(modes.begin(), std::find(modes.begin(), modes.end(), entry.mode())) + 1) % modes.size()]);
        } else if (m_editField == EditFieldHour) {
            entry.setTime((entry.time() + 60) % MINUTES_PER_DAY);
        } else {
            entry.setTime((entry.time() + 1) % MINUTES_PER_DAY);
        }
        break;
    case DisplayController::ButtonInput::BUTTON_PREVIOUS:
        if (m_editField == EditFieldMode) {
            entry.setMode(modes[(std::distance(modes.begin(), std::find(modes.begin(), modes.end(), entry.mode())) + modes.size() - 1) % modes.size()]);
        } else if (m_editField == EditFieldHour) {
            entry.setTime((entry.time() + MINUTES_PER_DAY - 60) % MINUTES_PER_DAY);
        } else {
            entry.setTime((entry.time() + MINUTES_PER_DAY - 1) % MINUTES_PER_DAY);
        }
        break;
    }

    Serial.println(String("New time: ") + entry.time() + " - " + timers.size() + " - " + m_selection);

    engine->updateTimer(m_selection, entry);
}

void TimersPage::reset() {
    m_needsRefresh = true;
    m_active = false;
    m_selection = 0;
    m_editMode = false;
    m_cycle = 0;
}

bool TimersPage::autoExit() const {
     return !m_editMode;
};
