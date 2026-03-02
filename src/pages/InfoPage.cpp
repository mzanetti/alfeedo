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

#include "InfoPage.h"

#include <WiFi.h>

InfoPage::InfoPage():
    Page("InfoPage")
{
}

void InfoPage::render(Adafruit_SSD1306 &display, Engine *engine, FillSensor *fillSensor, OTAManager *otaManager)
{
    if (!m_needsRefresh) {
        return;
    }
    display.clearDisplay();

    if (!m_active) {
        drawCenteredText(display, m_cycle, "Info", 2, "", 1);
    } else {
        if (!m_optionActive) {
            switch (m_currentOption) {
            case OptionVersion:
                drawMenu(display, m_cycle, 1,  "Info", 1, "Version", 2);
                break;
            case OptionNetwork:
                drawMenu(display, m_cycle, 1, "Info", 1, "Network", 2);
                break;
            }
        } else {
            switch (m_currentOption) {
            case OptionVersion:
                drawMenu(display, m_cycle, -1, "Version", 1, String(CATFEEDER_VERSION), 1);
                break;
            case OptionNetwork:
                drawMenu(display, m_cycle, -1, "Network", 1, "IP: " + WiFi.localIP().toString(), 1);
                break;
            }
        }
    }
    display.display();
}

void InfoPage::handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input)
{
    m_needsRefresh = true;

    if (!m_active) {
        if (input == DisplayController::ButtonInput::BUTTON_MENU) {
            m_active = true;
            return;
        } else {
            Page::handleInput(controller, engine, input);
            return;
        }
    } else {
        if (!m_optionActive) {
            switch (input){
            case DisplayController::ButtonInput::BUTTON_MENU:
                m_optionActive = true;
                break;
            case DisplayController::ButtonInput::BUTTON_NEXT:
                m_currentOption = static_cast<Option>((static_cast<int>(m_currentOption) + 1) % 2);
                break;
            case DisplayController::ButtonInput::BUTTON_PREVIOUS:
                m_currentOption = static_cast<Option>((static_cast<int>(m_currentOption) - 1 + 2) % 2);
                break;
            }
            return;
        } else {
            m_optionActive = false;
        }
    }
}

void InfoPage::reset()
{
    m_active = false;
    m_currentOption = OptionVersion;
    m_optionActive = false;
    m_needsRefresh = true;
    m_cycle = 0;
}