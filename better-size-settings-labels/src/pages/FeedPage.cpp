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

#include "FeedPage.h"

#include "Engine.h"

FeedPage::FeedPage():
    Page("FeedPage")
{
}

void FeedPage::render(Adafruit_SSD1306 &display, Engine *engine, FillSensor *fillSensor, OTAManager *otaManager)
{
    if (!m_needsRefresh) {
        return;
    }
    m_needsRefresh = false;

    display.clearDisplay();

    if (!m_active) {
        drawCenteredText(display, m_cycle, "Feed now", 2, "", 1);

    } else {
        if (m_selection == 0) {
            drawMenu(display, m_cycle, 1, "Feed now", 1, "Regular", 2);
        } else {
            drawMenu(display, m_cycle, 1, "Feed now", 1, "Snack", 2);
        }
    }

    display.display();
}

void FeedPage::handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input)
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
        switch (input) {
        case DisplayController::ButtonInput::BUTTON_MENU:
            if (m_selection == 0) {
                engine->feed();
            } else {
                engine->feed(Engine::FeedingMode::Snack);
            }
            m_active = false;
            exit(controller);
            break;
        case DisplayController::ButtonInput::BUTTON_NEXT:
            m_selection = (m_selection + 1) % 2;
            break;
        case DisplayController::ButtonInput::BUTTON_PREVIOUS:
            m_selection = (m_selection - 1 + 2) % 2;
            break;
        }
    }
}

void FeedPage::reset()
{
    m_active = false;
    m_needsRefresh = true;
    m_selection = 0;
    m_cycle = 0;
}