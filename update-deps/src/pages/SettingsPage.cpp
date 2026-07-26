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

#include "SettingsPage.h"
#include "MainPage.h"
#include "Engine.h"
#include "TimeSource.h"

#include <cstring>

std::vector<MainPage::HotKeyFunction> hotkeyList = {
    MainPage::HotKeyFunctionOff,
    MainPage::HotKeyFunctionFeed,
    MainPage::HotKeyFunctionSnack,
};

std::map<SettingsPage::ResetOption, String> SettingsPage::resetOptionMap = {
    {SettingsPage::ResetOption::No, "No"},
    {SettingsPage::ResetOption::Yes, "Yes"},
};

SettingsPage::SettingsPage(MainPage *mainPage):
    Page("SettingsPage")
{
    m_mainPage = mainPage;
}

void SettingsPage::setTimeSource(TimeSource *timeSource)
{
    m_timeSource = timeSource;
    m_timeSettingsPage.setTimeSource(timeSource);
}

void SettingsPage::setResetFunction(ResetFunction resetFunction) {
    m_resetFunction = resetFunction;
}

void SettingsPage::render(Adafruit_SSD1306 &display, Engine *engine, FillSensor *fillSensor, OTAManager *otaManager)
{
    if (needsTimedRefresh()) {
        m_needsRefresh = true;
        m_cycle++;
    }
    if (!m_needsRefresh) {
        return;
    }
    m_needsRefresh = false;
    display.clearDisplay();

    // Main entry point
    if (!m_active) {
        drawCenteredText(display, m_cycle, "Settings", 2, "", 1);
        display.display();
        return;

    }

    //1st level menu - option selection
    if (!m_optionActive) {
        switch (m_currentOption) {
        case Option::DateTime:
            drawMenu(display, m_cycle, 1, "Settings", 1, "Date & Time", 2);
            break;
        case Option::Speed:
            drawMenu(display, m_cycle, 1, "Settings", 1, "Speed", 2);
            break;
        case Option::MealSize:
            drawMenu(display, m_cycle, 1, "Settings", 1, "Meal size", 2);
            break;
        case Option::SnackSize:
            drawMenu(display, m_cycle, 1, "Settings", 1, "Snack size", 2);
            break;
        case Option::HotKey:
            drawMenu(display, m_cycle, 1, "Settings", 1, "Hotkey", 2);
            break;
        case Option::Reset:
            drawMenu(display, m_cycle, 1, "Settings", 1, "Reset", 2);
            break;
        }
        display.display();
        return;
    }

    int yOffset = 8 + 5; // offset for second line: text size + spacing

    // 2nd level menu - option details
    switch (m_currentOption) {
    case Option::DateTime:
        m_timeSettingsPage.render(display, engine, fillSensor, otaManager);
        return;
    case Option::Speed:
        display.setTextSize(1);
        drawScrollingLabel(display, m_cycle, "Speed", false, 0);
        drawBarControl(display, engine->speed(), engine->minSpeed(), engine->maxSpeed(), yOffset);
        break;
    case Option::MealSize: 
        display.setTextSize(1);
        drawScrollingLabel(display, m_cycle, "Portion", false, 0);
        drawBarControl(display, engine->revolutionsPerPortion(), engine->minRevolutionsPerPortion(), engine->maxRevolutionsPerPortion(), yOffset);
        break;
    case Option::SnackSize:
        display.setTextSize(1);
        drawScrollingLabel(display, m_cycle, "Snack", false, 0);
        drawBarControl(display, engine->revolutionsPerSnack(), engine->minRevolutionsPerPortion(), engine->maxRevolutionsPerPortion(), yOffset);
        break;
    case Option::HotKey:
        drawMenu(display, m_cycle, 1, "Hotkey function", 1, MainPage::hotkeys().at(m_mainPage->hotKeyFunction()), 2);
        break;
    case Option::Reset:
        drawMenu(display, m_cycle, 1, "Reset device?", 1, resetOptionMap.at(m_currentResetOption), 2);
        break; 
    }
    display.display();
}

void SettingsPage::handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input)
{
    m_cycle = 0;
    m_needsRefresh = true;
    if (!m_active) {
        if (input == DisplayController::ButtonInput::BUTTON_MENU) {
            m_active = true;
            return;
        } else {
            Page::handleInput(controller, engine, input);
            return;
        }
    }

    if (!m_optionActive) {
        switch (input){
        case DisplayController::ButtonInput::BUTTON_MENU:
            m_optionActive = true;
            break;
        case DisplayController::ButtonInput::BUTTON_NEXT:
            m_currentOption = static_cast<Option>((static_cast<int>(m_currentOption) + 1) % 6);
            break;
        case DisplayController::ButtonInput::BUTTON_PREVIOUS:
            m_currentOption = static_cast<Option>((static_cast<int>(m_currentOption) - 1 + 6) % 6);
            break;
        }
        return;
    }

    switch (m_currentOption) {
    case Option::DateTime:
        m_timeSettingsPage.handleInput(controller, engine, input);
        return;
    case Option::Speed:
        switch (input) {
        case DisplayController::ButtonInput::BUTTON_MENU:
            m_optionActive = false;
            return;
        case DisplayController::ButtonInput::BUTTON_NEXT:
            Serial.println("Increasing speed. Old: " +  String(engine->speed()));
            engine->setSpeed(min(engine->speed() + 0.1f, engine->maxSpeed()));
            return;
        case DisplayController::ButtonInput::BUTTON_PREVIOUS:
            engine->setSpeed(max(engine->speed() - 0.1f, engine->minSpeed()));
            return;
        }
        break;
    case Option::MealSize:
        switch (input) {
        case DisplayController::ButtonInput::BUTTON_MENU:
            m_optionActive = false;
            return;
        case DisplayController::ButtonInput::BUTTON_NEXT:
            engine->setRevolutionsPerPortion(min(engine->revolutionsPerPortion() + .25f, engine->maxRevolutionsPerPortion()));
            return;
        case DisplayController::ButtonInput::BUTTON_PREVIOUS:   
            engine->setRevolutionsPerPortion(max(engine->revolutionsPerPortion() - .25f, engine->minRevolutionsPerPortion()));
            return;
        }
        break;
    case Option::SnackSize:
        switch (input) {
        case DisplayController::ButtonInput::BUTTON_MENU:
            m_optionActive = false;
            return;
        case DisplayController::ButtonInput::BUTTON_NEXT:
            engine->setRevolutionsPerSnack(min(engine->revolutionsPerSnack() + .1f, engine->maxRevolutionsPerPortion()));
            return;
        case DisplayController::ButtonInput::BUTTON_PREVIOUS:
            engine->setRevolutionsPerSnack(max(engine->revolutionsPerSnack() - .1f, engine->minRevolutionsPerPortion()));
            return;
        }
        break;
    case Option::HotKey: {
        switch (input) {
        case DisplayController::ButtonInput::BUTTON_MENU:
            m_optionActive = false;
            return;
        case DisplayController::ButtonInput::BUTTON_NEXT:
            m_mainPage->setHotKeyFunction((MainPage::HotKeyFunction)(((int)m_mainPage->hotKeyFunction() + 1) % hotkeyList.size()));
            return;
        case DisplayController::ButtonInput::BUTTON_PREVIOUS:
            m_mainPage->setHotKeyFunction((MainPage::HotKeyFunction)(((int)m_mainPage->hotKeyFunction() - 1 + hotkeyList.size()) % hotkeyList.size()));
            return;
        }
        break;
    }
    case Option::Reset:
        switch (input) {
        case DisplayController::ButtonInput::BUTTON_MENU:
            if (m_currentResetOption == ResetOption::Yes) {
                m_resetFunction();
            }
            m_optionActive = false;
            return;
        case DisplayController::ButtonInput::BUTTON_NEXT:
            m_currentResetOption = (SettingsPage::ResetOption)(((int)m_currentResetOption + 1) % 2);
            return;
        case DisplayController::ButtonInput::BUTTON_PREVIOUS:
            m_currentResetOption = (SettingsPage::ResetOption)(((int)m_currentResetOption - 1 + 2) % 2);
            return;
        }
        break;
    }   

}

void SettingsPage::reset()
{
    m_active = false;
    m_currentOption = Option::DateTime;
    m_needsRefresh = true;
    m_cycle = 0;
    m_timeSettingsPage.reset();
}

bool SettingsPage::autoExit() const
{
    if (m_currentOption == Option::DateTime && m_timeSettingsPage.autoExit()) {
        return true;
    }
    return !m_optionActive;
}
