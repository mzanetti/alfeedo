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

#include "TimeSettingsPage.h"
#include "MainPage.h"
#include "Engine.h"
#include "TimeSource.h"

#include <cstring>


TimeSettingsPage::TimeSettingsPage():
    Page("TimeSettingsPage")
{
}

void TimeSettingsPage::setTimeSource(TimeSource *timeSource)
{
    m_timeSource = timeSource;
}

void TimeSettingsPage::render(Adafruit_SSD1306 &display, Engine *engine, FillSensor *fillSensor, OTAManager *otaManager)
{
    static bool oldBlinkOn = true;
    bool blinkOn = (millis() / 500) % 2 == 0;
    if (oldBlinkOn != blinkOn) {
        oldBlinkOn = blinkOn;
        m_needsRefresh = true;
    }

    if (m_currentOption == DateTimeOption::TimeZone && m_optionActive && needsTimedRefresh()) {
        m_needsRefresh = true;
        m_cycle++;
    }

    if (!m_needsRefresh) {
        return;
    }
    m_needsRefresh = false;
    display.clearDisplay();


    if (!m_optionActive) {
        std::unordered_map<DateTimeOption, String> optionText = {
            {DateTimeOption::TimeZone, "Time Zone"},
            {DateTimeOption::DateTime, "Date & Time"},
        };
        drawMenu(display, m_cycle, 1, "Time Settings", 1, optionText[m_currentOption], 2);
        display.display();
        return;
    }

    int yOffset = 8 + 5; // offset for second line: text size + spacing

    switch (m_currentOption) {
    case DateTimeOption::TimeZone: {
        TimeSource::TimeZoneEntry tz = m_timeSource->timeZone();
        drawMenu(display, m_cycle, 1, "Time Zone", 1, String(tz.friendlyName), 1);
        break;
    }
    case DateTimeOption::DateTime: {
        Serial.println("Rendering DateTime option, current editing time: " + String(asctime(&m_editingDateTime)));
        char yearString[11];        
        sprintf(yearString, "%02d.%02d.%02d", m_editingDateTime.tm_mday, m_editingDateTime.tm_mon + 1, m_editingDateTime.tm_year + 1900);
        char timeString[6];        
        sprintf(timeString, "%02d:%02d", m_editingDateTime.tm_hour, m_editingDateTime.tm_min);
        int line1Size = 1;
        int line2Size = 1;
        switch (m_currentDateTimeEditField) {
        case DateTimeEditField::Hour:
        case DateTimeEditField::Minute:
            line2Size = 2;
            break;
        default:
            line1Size = 2;
            break;
        }

        if (!blinkOn) {
            switch (m_currentDateTimeEditField) {
            case DateTimeEditField::Day:
                sprintf(yearString, "  .%02d.%04d", m_editingDateTime.tm_mon + 1, m_editingDateTime.tm_year + 1900);
                break;
            case DateTimeEditField::Month:
                sprintf(yearString, "%02d.  .%04d", m_editingDateTime.tm_mday, m_editingDateTime.tm_year + 1900);
                break;
            case DateTimeEditField::Year:
                sprintf(yearString, "%02d.%02d.    ", m_editingDateTime.tm_mday, m_editingDateTime.tm_mon + 1);
                break;
            case DateTimeEditField::Hour:
                sprintf(timeString, "  :%02d", m_editingDateTime.tm_min);
                break;
            case DateTimeEditField::Minute:
                sprintf(timeString, "%02d:  ", m_editingDateTime.tm_hour);
                break;
            }
        }    
        drawMenu(display, m_cycle, -1, yearString, line1Size, timeString, line2Size);
        break;
    }
    }

    display.display();
}

void TimeSettingsPage::handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input)
{
    m_needsRefresh = true;
    m_cycle = 0;

    if (!m_optionActive) {
        switch (input) {
        case DisplayController::ButtonInput::BUTTON_MENU:
            m_optionActive = true;
            m_editingDateTime = m_timeSource->currentDateTime();
            return;
        case DisplayController::ButtonInput::BUTTON_NEXT:
            m_currentOption = static_cast<DateTimeOption>((static_cast<int>(m_currentOption) + 1) % 2);
            return;
        case DisplayController::ButtonInput::BUTTON_PREVIOUS:
            m_currentOption = static_cast<DateTimeOption>((static_cast<int>(m_currentOption) - 1 + 2) % 2);
            return;
        }
        return;
    }

    switch (m_currentOption) {
    case DateTimeOption::TimeZone:
        switch (input) {
        case DisplayController::ButtonInput::BUTTON_MENU:
            m_optionActive = false;
            return;
        case DisplayController::ButtonInput::BUTTON_NEXT:
            cycleTimeZone(1);
            return;
        case DisplayController::ButtonInput::BUTTON_PREVIOUS:
            cycleTimeZone(-1);
            return;
        }
        break;
    case DateTimeOption::DateTime:
        switch (input) {
        case DisplayController::ButtonInput::BUTTON_MENU:
            if (m_currentDateTimeEditField != DateTimeEditField::Minute) {
                m_currentDateTimeEditField = static_cast<DateTimeEditField>((static_cast<int>(m_currentDateTimeEditField) + 1) % 5);
            } else {
                m_timeSource->setCurrentDateTime(m_editingDateTime);
                m_optionActive = false;
                m_currentDateTimeEditField = DateTimeEditField::Day;
            }
            return;
        case DisplayController::ButtonInput::BUTTON_NEXT:
            switch (m_currentDateTimeEditField) {
            case DateTimeEditField::Year:
                m_editingDateTime.tm_year += 1;
                break;
            case DateTimeEditField::Month:
                m_editingDateTime.tm_mon += 1;
                break;
            case DateTimeEditField::Day:
                m_editingDateTime.tm_mday += 1;
                break;
            case DateTimeEditField::Hour:
                m_editingDateTime.tm_hour += 1;
                break;
            case DateTimeEditField::Minute:
                m_editingDateTime.tm_min += 1;
                break;  
            }
            return;
        case DisplayController::ButtonInput::BUTTON_PREVIOUS:
            switch (m_currentDateTimeEditField) {
            case DateTimeEditField::Year:
                m_editingDateTime.tm_year -= 1;
                break;
            case DateTimeEditField::Month:
                m_editingDateTime.tm_mon -= 1;
                break;
            case DateTimeEditField::Day:    
                m_editingDateTime.tm_mday -= 1;
                break;
            case DateTimeEditField::Hour:
                m_editingDateTime.tm_hour -= 1;
                break;
            case DateTimeEditField::Minute:
                m_editingDateTime.tm_min -= 1;
                break;
            }
            return;
        }   
    }
}

void TimeSettingsPage::reset()
{
    m_optionActive = false;
    m_currentOption = DateTimeOption::TimeZone;
    
    m_needsRefresh = true;
    m_cycle = 0;
}

void TimeSettingsPage::cycleTimeZone(int steps)
{
    if (steps == 0) {
        return;
    }

    TimeSource::TimeZoneEntry currentTZ = m_timeSource->timeZone();
    std::vector<TimeSource::TimeZoneEntry> list = TimeSource::timeZoneList();

    size_t listSize = list.size();

    if (listSize == 0) {
        Serial.println("Error: TimeZone list is empty.");
        return;
    }

    auto it = std::find_if(list.begin(), list.end(),
                            [&currentTZ](const TimeSource::TimeZoneEntry &entry) {
                                return std::strcmp(entry.tzString, currentTZ.tzString) == 0;
                            });

    size_t currentIndex = 0;
    if (it != list.end()) {
        currentIndex = std::distance(list.begin(), it);
    } else {
        Serial.printf("Warning: Current TZ '%s' not found. Starting cycle from first entry.\n", currentTZ.tzString);
    }

    long newIndex = (long)currentIndex + steps;
    long listSizeLong = (long)listSize;

    newIndex = newIndex % listSizeLong;
    if (newIndex < 0) {
        newIndex += listSizeLong;
    }

    TimeSource::TimeZoneEntry nextTZ = list[(size_t)newIndex];
    m_timeSource->setTimeZone(nextTZ);

    Serial.print("Timezone cycled by ");
    Serial.print(steps);
    Serial.print(" steps to: ");
    Serial.println(nextTZ.friendlyName);
    
}

bool TimeSettingsPage::autoExit() const
{
    return !m_optionActive;
}

