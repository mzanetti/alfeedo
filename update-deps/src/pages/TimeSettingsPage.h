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

#ifndef TIMESETTINGS_PAGE_H
#define TIMESETTINGS_PAGE_H

#include "pages/Page.h"
#include "ResetHandler.h"

#include <map>

class MainPage;
class TimeSource;

class TimeSettingsPage: public Page {
public:
    TimeSettingsPage();
    void render(Adafruit_SSD1306 &display, Engine *engine, FillSensor *fillSensor, OTAManager *otaManager) override;
    void handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input) override;
    void reset() override;
    bool autoExit() const override;

    void setTimeSource(TimeSource* timeSource);
private:

    enum class DateTimeOption {
        TimeZone,
        DateTime
    };

    enum class DateTimeEditField {
        Day,
        Month,
        Year,
        Hour,
        Minute
    };

    void cycleTimeZone(int steps);

    bool m_needsRefresh = true;
    int m_cycle = 0;
    bool m_optionActive = false;
    DateTimeOption m_currentOption = DateTimeOption::TimeZone;
    DateTimeEditField m_currentDateTimeEditField = DateTimeEditField::Day;
    TimeSource *m_timeSource = nullptr;
    struct tm m_editingDateTime = {0};

};

#endif // TIMESETTINGS_PAGE_H