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

#ifndef TIMESOURCE_H
#define TIMESOURCE_H

#include <Arduino.h>

#include <Preferences.h>

#include <vector>

class TimeSource
{
public:
    struct TimeZoneEntry {
        const char *friendlyName;
        const char *tzString;
    };

    void begin();
    void loop();

    void reset();

    bool ntpSynchronized() const;

    struct tm currentDateTime() const;
    String currentTimeString() const;
    String currentDateTimeString() const;
    void setCurrentDateTime(struct tm localtime);

    TimeZoneEntry timeZone() const;
    bool setTimeZone(const TimeZoneEntry &timeZone);
    bool setTimeZone(const String &tzString);

    static std::vector<TimeZoneEntry> timeZoneList();

private:
    void saveCurrentTime();
    void applyTimeZone();

    Preferences m_prefs;
    time_t m_lastSavedTime = 0;
    TimeZoneEntry m_timeZone;

};

#endif