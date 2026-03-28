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

#ifndef ENGINE_H
#define ENGINE_H

#include "Motor.h"
#include "TimerEntry.h"

#include <Preferences.h>

#include <vector>

class FillSensor;
class TimeSource;

class Engine {
  public:

    enum class FeedingMode {
        Meal,
        Snack,
    };

    enum class State {
        Idle,
        Moving
    };

    enum class ErrorState {
        NoError,
        Struggling,
        Jammed
    };

    struct LogEntry {
        enum Type {
            TypeManualFeed,
            TypeTimerFeed,
        };
        time_t timestamp;
        Type type;
        FeedingMode mode;
        int stallCount;
        bool aborted;
        int fillLevel;
    };

    static constexpr int MaxTimers = 10;

    Engine();
    ~Engine();

    void begin(TimeSource *timeSource, Motor *motor, FillSensor *fillSensor);
    void update();
    void reset();

    void feed(FeedingMode mode = FeedingMode::Meal);

    State state() const;
    ErrorState errorState() const;
    int lastStallCount() const;

    std::vector<TimerEntry> timers() const;
    bool addTimer(const TimerEntry &entry);
    bool updateTimer(uint8_t index, const TimerEntry &entry);
    bool removeTimer(uint8_t index);
    TimerEntry nextTimer() const;

    float speed() const;
    float minSpeed() const;
    float maxSpeed() const;
    void setSpeed(float speed);

    float revolutionsPerPortion() const;
    float minRevolutionsPerPortion() const;
    float maxRevolutionsPerPortion() const;
    void setRevolutionsPerPortion(float revolutions);

    float revolutionsPerSnack() const;
    void setRevolutionsPerSnack(float revolutions);

    std::vector<LogEntry> logEntries() const;
    void clearLogs();


private:
    void storeTimer(int index, bool openCloseSettings = true);
    void feed(LogEntry::Type type, FeedingMode mode);
    void addLogEntry(time_t timestamp, LogEntry::Type type, FeedingMode mode, int stallCount, bool aborted);
    
    TimeSource *m_timeSource = nullptr;
    Motor* m_motor = nullptr;
    FillSensor* m_fillSensor = nullptr;
    float m_revolutionsPerMeal = 0;
    float m_revolutionsPerSnack = 0;
    float m_speed = 0;
    mutable Preferences m_prefs;

    std::vector<TimerEntry> m_timers;

    struct CurrentOperation {
        bool inProgress = false;
        LogEntry::Type type;
        FeedingMode mode;
        time_t startTime;
        int stallCountAtStart;
    };
    CurrentOperation m_currentOperation;
    int m_lastStallCount = 0;
    TimerEntry m_nextTimer;
};

#endif // ENGINE_H