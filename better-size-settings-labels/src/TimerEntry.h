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

#ifndef TIMERENTRY_H
#define TIMERENTRY_H

#include <Arduino.h>

class Engine;

class TimerEntry {
  public:
    enum class TimerMode {
        Off,
        Meal,
        Snack
    };
    TimerEntry() = default;
    TimerEntry(uint16_t time, TimerMode mode);

    uint16_t time() const;
    void setTime(uint16_t time);
    String timeString() const;

    TimerMode mode() const;
    void setMode(TimerMode mode);

    void operator=(const TimerEntry &other);

    static String timerModeToString(TimerMode mode);
    static TimerMode timerModeFromString(const String &mode);

  private:
    friend class Engine;
    void setLastTriggered(time_t lastTriggered);
    time_t lastTriggered() const;
    String lastTriggeredString() const;

    uint16_t m_time = 0;
    TimerMode m_mode = TimerMode::Off;
    time_t m_lastTriggered = 0;
};

#endif