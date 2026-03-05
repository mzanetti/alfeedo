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

#ifndef INFOPAGE_H
#define INFOPAGE_H

#include "Page.h"

class InfoPage: public Page
{
public:
    InfoPage();
    void render(Adafruit_SSD1306 &display, Engine *engine, FillSensor *fillSensor, OTAManager *otaManager) override;
    void handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input) override;

    void reset();

private:
    enum Option
    {
        OptionVersion,
        OptionNetwork
    };
    bool m_active = false;
    bool m_needsRefresh = true;
    Option m_currentOption = OptionVersion;
    bool m_optionActive = false;
    int m_cycle = 0;
};

#endif // INFOPAGE_H