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

#ifndef MAINPAGE_H
#define MAINPAGE_H 

#include "pages/Page.h"

#include <Preferences.h>

#include <map>

class MainPage: public Page {
public:
    enum HotKeyFunction {
        HotKeyFunctionOff = 0,
        HotKeyFunctionFeed = 1,
        HotKeyFunctionSnack = 2
    };

    MainPage();
    void begin();
    void render(Adafruit_SSD1306 &display, Engine *engine, FillSensor *fillSensor, OTAManager *otaManager) override;
    void handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input) override;
    bool autoExit() const override { return false; }

    HotKeyFunction hotKeyFunction() const;
    void setHotKeyFunction(HotKeyFunction hotKeyFunction);

    static std::map<HotKeyFunction, String> hotkeys() {
        return {
            {MainPage::HotKeyFunctionOff, "Off"},
            {MainPage::HotKeyFunctionFeed, "Feed"},
            {MainPage::HotKeyFunctionSnack, "Snack"}
        };
    }

  private:
    Preferences m_prefs;

    HotKeyFunction m_hotKeyFunction = HotKeyFunctionFeed;
};

#endif // MAINPAGE_H