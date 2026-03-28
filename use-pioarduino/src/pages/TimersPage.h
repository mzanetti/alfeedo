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

#ifndef FEEDINGTIMESPAGE_H
#define FEEDINGTIMESPAGE_H

#include "Page.h"

class TimersPage: public Page
{
public:
    TimersPage();
    void render(Adafruit_SSD1306 &display, Engine *engine, FillSensor *FillSensor, OTAManager *otaManager) override;
    void handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input) override;
    void reset() override;
    bool autoExit() const override;

private:
    enum EditField {
        EditFieldMode,
        EditFieldHour,
        EditFieldMinute
    };
    bool m_needsRefresh = false;
    bool m_active = false;
    uint8_t m_selection = 0;
    bool m_editMode = false;
    EditField m_editField = EditFieldMode;
    bool m_cycle = 0;
};

#endif