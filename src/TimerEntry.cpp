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

#include "TimerEntry.h"

#include <Arduino.h>
#include <unordered_map>

namespace std {
  template <>
  struct hash<String> {
    size_t operator()(const String& s) const {
      return std::hash<std::string>{}(s.c_str());
    }
  };
}

TimerEntry::TimerEntry(uint16_t time, TimerMode mode) : 
    m_time(time),
    m_mode(mode)
{
}

uint16_t TimerEntry::time() const {
    return m_time;
}

void TimerEntry::setTime(uint16_t time) {
    m_time = time;
}

TimerEntry::TimerMode TimerEntry::mode() const {
    return m_mode;
}

void TimerEntry::setMode(TimerEntry::TimerMode mode) {
    m_mode = mode;
}

time_t TimerEntry::lastTriggered() const {
    return m_lastTriggered;
}

void TimerEntry::setLastTriggered(time_t lastTriggered) {
    m_lastTriggered = lastTriggered;
}

String TimerEntry::timeString() const
{
    char timeString[6];
    int8_t hours = floor(m_time / 60);
    int8_t minutes = m_time % 60;
    sprintf(timeString, "%02d:%02d", hours, minutes);
    return String(timeString);
}

String TimerEntry::lastTriggeredString() const
{
    if (m_lastTriggered == 0) {
        return "-";
    }
    tm localtime;
    time_t trigger = m_lastTriggered;
    localtime_r(&trigger, &localtime);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localtime);
    return String(buffer);
}

void TimerEntry::operator=(const TimerEntry & other)
{
    m_time = other.time();
    m_mode = other.mode();
    m_lastTriggered = other.lastTriggered();
}

String TimerEntry::timerModeToString(TimerMode mode) {
    std::unordered_map<TimerEntry::TimerMode, String> modeMap = {
        {TimerEntry::TimerMode::Off, "off"},
        {TimerEntry::TimerMode::Meal, "meal"},
        {TimerEntry::TimerMode::Snack, "snack"}
    };

    return modeMap.at(mode);
}

TimerEntry::TimerMode TimerEntry::timerModeFromString(const String &mode) {
    std::unordered_map<String, TimerEntry::TimerMode> modeMap = {
        {"off", TimerEntry::TimerMode::Off},
        {"meal", TimerEntry::TimerMode::Meal},
        {"snack", TimerEntry::TimerMode::Snack}
    };
    if (modeMap.find(mode) == modeMap.end()) {
        return TimerEntry::TimerMode::Off;
    }
    return modeMap.at(mode);
}
