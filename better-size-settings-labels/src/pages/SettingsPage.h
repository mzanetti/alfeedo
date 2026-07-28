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

#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

#include "pages/Page.h"
#include "TimeSettingsPage.h"
#include "ResetHandler.h"

#include <map>

class MainPage;
class TimeSource;

class SettingsPage: public Page {
public:
    SettingsPage(MainPage *mainPage);
    void render(Adafruit_SSD1306 &display, Engine *engine, FillSensor *fillSensor, OTAManager *otaManager) override;
    void handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input) override;
    void reset() override;
    bool autoExit() const override;

    void setTimeSource(TimeSource* timeSource);
    void setResetFunction(ResetFunction resetFunction);
private:
    enum class Option {
        MealSize,
        SnackSize,
        Speed,
        DateTime,
        HotKey,
        Reset
    };

    enum class ResetOption {
        No,
        Yes
    };
    static std::map<SettingsPage::ResetOption, String> resetOptionMap;

    MainPage *m_mainPage = nullptr;
    TimeSource *m_timeSource = nullptr;
    bool m_active = false;
    bool m_needsRefresh = true;
    Option m_currentOption = Option::MealSize;
    ResetOption m_currentResetOption = ResetOption::No;
    bool m_optionActive = false;
    int m_cycle = 0;
    ResetFunction m_resetFunction;

    TimeSettingsPage m_timeSettingsPage;
};

#endif // SETTINGS_PAGE_H