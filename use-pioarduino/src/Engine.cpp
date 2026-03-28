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

#include "Engine.h"
#include "Logging.h"
#include "FillSensor.h"
#include "TimeSource.h"

LoggingCategory lcEngine("Engine");

constexpr float MIN_SPEED = 0.5;
constexpr float MAX_SPEED = 1.5;
constexpr float MIN_PORTION = .25;
constexpr float MAX_PORTION = 5;

Engine::Engine() {
    // Constructor implementation
}

Engine::~Engine() {
    // Destructor implementation
}

void Engine::begin(TimeSource *timeSource, Motor *motor, FillSensor *fillSensor) {
    m_timeSource = timeSource;
    m_motor = motor;
    m_fillSensor = fillSensor;

    m_prefs.begin("catfeeder", true); // read-only
    m_revolutionsPerMeal = m_prefs.getFloat("revsPerMeal", 2);
    m_revolutionsPerSnack = m_prefs.getFloat("revsPerSnack", .2);
    m_speed = m_prefs.getFloat("speed", 1);
    m_prefs.end();

    logInfo(lcEngine, "Loaded settings - revolutionsPerMeal: " + String(m_revolutionsPerMeal) + ", revolutionsPerSnack: " + String(m_revolutionsPerSnack) + ", speed: " + String(m_speed));

    m_prefs.begin("timers");
    int count = m_prefs.getInt("count", 0);
    for (int i = 0; i < count; i++) {
        String key = "time" + String(i);
        uint16_t time = m_prefs.getInt(key.c_str(), -1);
        key = "mode" + String(i);
        TimerEntry::TimerMode mode = (TimerEntry::TimerMode)m_prefs.getInt(key.c_str(), 0);
        key = "lastTriggered" + String(i);
        time_t lastTriggered = m_prefs.getLong(key.c_str(), 0);

        TimerEntry entry {time, mode};
        entry.setLastTriggered(lastTriggered);
        m_timers.push_back(entry);
    }
    m_prefs.end();

    if (m_motor) {
        m_motor->begin();

        m_motor->setSpeed(m_speed);
    }

}

void Engine::update() {
    
    struct tm currentDateTime = m_timeSource->currentDateTime();
    int currentMinutes = (currentDateTime.tm_hour * 60) + currentDateTime.tm_min;
    time_t now = mktime(&currentDateTime);
    time_t startOfDay = now - currentDateTime.tm_sec - (currentDateTime.tm_min * 60) - (currentDateTime.tm_hour * 3600);

    TimerEntry nextToday(24*60, TimerEntry::TimerMode::Off);
    TimerEntry nextTomorrow(24*60, TimerEntry::TimerMode::Off);

    for (int i = 0; i < m_timers.size(); i++) {
        TimerEntry &entry = m_timers.at(i);

        if (currentMinutes >= entry.time()) {
            if (entry.lastTriggered() < startOfDay) {
                logInfo(lcEngine, "FEEDING TRIGGERED! Current time: " + m_timeSource->currentTimeString() + ", Timer: " + entry.timeString() + ", Last triggered: " + String(entry.lastTriggered()) + ", StartOfDay: " + String(startOfDay));

                switch (entry.mode()) {
                case TimerEntry::TimerMode::Meal:
                    feed(LogEntry::TypeTimerFeed, FeedingMode::Meal);
                    break;
                case TimerEntry::TimerMode::Snack:
                    feed(LogEntry::TypeTimerFeed, FeedingMode::Snack);
                    break;
                case TimerEntry::TimerMode::Off:
                    logInfo(lcEngine, "Timer is off. not triggering");
                    break;
                } 
                entry.setLastTriggered(now);
                storeTimer(i);
            }
            if (entry.mode() != TimerEntry::TimerMode::Off) {
                if (nextTomorrow.mode() == TimerEntry::TimerMode::Off || entry.time() < nextTomorrow.time()) {
                    nextTomorrow = entry;
                }
            }
        } else {
            if (entry.mode() != TimerEntry::TimerMode::Off) {
                if (nextToday.mode() == TimerEntry::TimerMode::Off || entry.time() < nextToday.time()) {
                    nextToday = entry;
                }
            }
        }
    }

    if (nextToday.mode() != TimerEntry::TimerMode::Off) {
        m_nextTimer = nextToday;
    } else if (nextTomorrow.mode() != TimerEntry::TimerMode::Off) {
        m_nextTimer = nextTomorrow;
    } else {
        m_nextTimer.setMode(TimerEntry::TimerMode::Off);
    }

    if (m_currentOperation.inProgress) {
        if (m_motor->stallCount() - m_currentOperation.stallCountAtStart != m_lastStallCount) {
            m_lastStallCount = m_motor->stallCount() - m_currentOperation.stallCountAtStart;
            logInfo(lcEngine, "Last stall count: " + String(m_lastStallCount));
        }

        if (!m_motor->isMoving()) {
            m_currentOperation.inProgress = false;
            
            int stallCount = m_motor->stallCount() - m_currentOperation.stallCountAtStart;
            addLogEntry(m_currentOperation.startTime, m_currentOperation.type, m_currentOperation.mode, stallCount, false);
            logInfo(lcEngine, "Feed operation completed: Mode: " + String(m_currentOperation.mode == FeedingMode::Meal ? "portion" : "snack") + ", Type: " + String(m_currentOperation.type == LogEntry::TypeManualFeed ? "manual" : "timer") + ", Duration: " + String(time(nullptr) - m_currentOperation.startTime) + " seconds, Stall count: " + String(stallCount));
        }
    }
}

void Engine::reset() {
    logInfo(lcEngine, "Resetting settings...");
    m_prefs.begin("catfeeder", false);
    m_prefs.clear();
    m_prefs.end();
    m_prefs.begin("timers");
    m_prefs.clear();
    m_prefs.end();
    m_timers.clear();
    m_prefs.begin("logs");
    m_prefs.clear();
    m_prefs.end();
    m_revolutionsPerMeal = 2;
    m_revolutionsPerSnack = .2;
    m_speed = 0;
}

void Engine::feed(FeedingMode mode) {
    feed(LogEntry::TypeManualFeed, mode);
}

Engine::State Engine::state() const {
    if (m_motor->isMoving()) {
        return State::Moving;
    }
    return State::Idle;
}

Engine::ErrorState Engine::errorState() const {
    if (m_lastStallCount > 3) {
        return ErrorState::Jammed;
    }
    if (m_lastStallCount > 1) {
        return ErrorState::Struggling;
    }
    return ErrorState::NoError;
}

int Engine::lastStallCount() const {
    return m_lastStallCount;
}

float Engine::speed() const
{
    return m_speed;
}

float Engine::minSpeed() const
{
    return MIN_SPEED;
}

float Engine::maxSpeed() const
{
    return MAX_SPEED;
}

void Engine::setSpeed(float speed)
{
    if (speed < minSpeed() || speed > maxSpeed()) {
        logWarning(lcEngine, "Speed setting out of range: " + String(speed) + " (" + String(minSpeed()) + " - " + String(maxSpeed()) + ")");
        return;
    }
    logInfo(lcEngine, "Setting speed to: " + String(speed));
    m_speed = speed;
    m_prefs.begin("catfeeder", false);
    m_prefs.putFloat("speed", speed);
    m_prefs.end();
}

void Engine::setRevolutionsPerPortion(float revolutions)
{
    if (revolutions < minRevolutionsPerPortion() || revolutions > maxRevolutionsPerPortion()) {
        logWarning(lcEngine, "Revolutions per meal setting out of range: " + String(revolutions) + " (" + String(minRevolutionsPerPortion()) + " - " + String(maxRevolutionsPerPortion()) + ")");
        return;
    }
    m_revolutionsPerMeal = revolutions;
    logInfo(lcEngine, "Revolutions per meal set to " + String(revolutions));
    m_prefs.begin("catfeeder", false);
    m_prefs.putFloat("revsPerMeal", revolutions);
    m_prefs.end();
}

float Engine::revolutionsPerPortion() const {
    return m_revolutionsPerMeal;
}

float Engine::revolutionsPerSnack() const
{
    return m_revolutionsPerSnack;
}

float Engine::minRevolutionsPerPortion() const
{
    return MIN_PORTION;
}

float Engine::maxRevolutionsPerPortion() const
{
    return MAX_PORTION;
}

void Engine::setRevolutionsPerSnack(float revolutionsPerSnack)
{
    if (revolutionsPerSnack < minRevolutionsPerPortion() || revolutionsPerSnack > maxRevolutionsPerPortion()) {
        logWarning(lcEngine, "Revolutions per portion setting out of range: " + String(revolutionsPerSnack) + " (" + String(minRevolutionsPerPortion()) + " - " + String(maxRevolutionsPerPortion()) + ")");
        return;
    }
    
    m_revolutionsPerSnack = revolutionsPerSnack;
    logInfo(lcEngine, "Revolutions per snack set to " + String(revolutionsPerSnack));
    m_prefs.begin("catfeeder", false);
    m_prefs.putFloat("revsPerSnack", revolutionsPerSnack);
    m_prefs.end();
}

std::vector<Engine::LogEntry> Engine::logEntries() const {

    std::vector<LogEntry> entries;
    m_prefs.begin("logs", true);
    size_t start = m_prefs.getUInt("start", 0);
    size_t count = m_prefs.getUInt("count", 0);

    for (int i = start; i < start + count; i++) {
        String baseKey = "log" + String(i) + "_";
        time_t timestamp = (time_t)m_prefs.getUInt((baseKey + "time").c_str(), 0);
        LogEntry::Type type = (LogEntry::Type)m_prefs.getUInt((baseKey + "type").c_str(), 0);
        FeedingMode mode = (FeedingMode)m_prefs.getUInt((baseKey + "mode").c_str(), 0);
        int stallCount = m_prefs.getInt((baseKey + "stalls").c_str(), 0);
        int fillLevel = m_prefs.getInt((baseKey + "fill").c_str(), -1);
        bool aborted = m_prefs.getBool((baseKey + "aborted").c_str(), false);
        LogEntry entry {timestamp, type, mode, stallCount, aborted, fillLevel};
        entries.push_back(entry);
    }
    m_prefs.end();

    return entries;
}

void Engine::clearLogs() {
    m_prefs.begin("logs", false);
    m_prefs.clear();
    m_prefs.end();
}

std::vector<TimerEntry> Engine::timers() const
{
    return m_timers;
}
bool Engine::addTimer(const TimerEntry &entry)
{
    if (m_timers.size() >= MaxTimers) {
        logWarning(lcEngine, "Cannot add timer, max count reached: " + String(m_timers.size()));
        return false;
    }

    m_timers.push_back(entry);

    // if timer is in the past, set lastTriggered to now to not trigger right away
    time_t now;
    time(&now);
    tm timeInfo;
    localtime_r(&now, &timeInfo);
    int currentMinutes = (timeInfo.tm_hour * 60) + timeInfo.tm_min;
    if (currentMinutes > entry.time()) {
        m_timers.at(m_timers.size() - 1).setLastTriggered(now);
    }

    logInfo(lcEngine, "Added new timer: " + entry.timeString() + ", " + TimerEntry::timerModeToString(entry.mode()));

    uint8_t index = m_timers.size() - 1;
    m_prefs.begin("timers");
    storeTimer(index, false);
    m_prefs.putInt("count", m_timers.size());
    m_prefs.end();

    return true;
}

bool Engine::updateTimer(uint8_t index, const TimerEntry &entry)
{
    if (index >= m_timers.size() || index < 0) {
        logWarning(lcEngine, "Invalid timer index " + String(index) + " Count: " + String(m_timers.size()));
        return false;
    }

    m_timers.at(index).setTime(entry.time());
    m_timers.at(index).setMode(entry.mode());

    // if timer is in the past, set lastTriggered to now to not trigger right away
    time_t now;
    time(&now);
    tm timeInfo;
    localtime_r(&now, &timeInfo);
    int currentMinutes = (timeInfo.tm_hour * 60) + timeInfo.tm_min;
    if (currentMinutes > entry.time()) {
        m_timers.at(index).setLastTriggered(now);
    } else {
        m_timers.at(index).setLastTriggered(0);
    }

    storeTimer(index);
    return true;
}


bool Engine::removeTimer(uint8_t index) {
    if (index >= m_timers.size() || index < 0) {
        logWarning(lcEngine, "Invalid timer index " + String(index) + " Count: " + String(m_timers.size()));
        return false;
    }
    logInfo(lcEngine, "Removing timer at index: " + String(index) + " (size: " + String(m_timers.size()) + ")");

    m_timers.erase(m_timers.begin() + index);

    m_prefs.begin("timers");
    m_prefs.clear();
    for (int i = 0; i < m_timers.size(); i++) {
        storeTimer(i, false);
    }
    m_prefs.putInt("count", m_timers.size());
    m_prefs.end();
    return true;
}

TimerEntry Engine::nextTimer() const {
    return m_nextTimer;
}

void Engine::storeTimer(int index, bool openCloseSettings)
{
    logInfo(lcEngine, "Storing timer at index " + String(index) + " (size: " + String(m_timers.size()) + ")");
    TimerEntry entry = m_timers.at(index);

    if (openCloseSettings) {
        m_prefs.begin("timers");
    }

    String key = "time" + String(index);
    m_prefs.putInt(key.c_str(), entry.time());
    key = "mode" + String(index);
    m_prefs.putInt(key.c_str(), (int)entry.mode());
    key = "lastTriggered" + String(index);
    m_prefs.putLong(key.c_str(), entry.lastTriggered());

    if (openCloseSettings) {
        m_prefs.end();
    }
}

void Engine::feed(LogEntry::Type type, FeedingMode mode) {
    if (!m_motor) {
        logError(lcEngine, "Motor not set");
        return;
    }
    if (m_revolutionsPerMeal <= 0) {
        logError(lcEngine, "Cannot feed, not calibrated");
        return;
    }

    if (m_currentOperation.inProgress) {
        logWarning(lcEngine, "Aborting running feed operation to start new one.");
        int stallCount = m_motor->stallCount() - m_currentOperation.stallCountAtStart;
        addLogEntry(m_currentOperation.startTime, m_currentOperation.type, m_currentOperation.mode, stallCount, true);
    }

    float revs = (mode == FeedingMode::Meal) ? m_revolutionsPerMeal : m_revolutionsPerSnack;

    logInfo(lcEngine, String("Engine: Feeding ") + (mode == FeedingMode::Meal ? "meal" : "snack") + " ("  + String(revs) + " revs), triggered by " + (type == LogEntry::TypeManualFeed ? "manual feed" : "timer"));
    m_currentOperation.type = type;
    m_currentOperation.mode = mode;
    m_currentOperation.stallCountAtStart = m_motor->stallCount();
    m_currentOperation.startTime = time(nullptr);
    m_currentOperation.inProgress = true;
    m_motor->move(revs);

}

void Engine::addLogEntry(time_t timestamp, LogEntry::Type type, FeedingMode mode, int stallCount, bool aborted) {
    
    /*     
    The ESP flash storage can hold approx 60 logs at time of measuring. This means we require some strategy to
    expire old logs. As we don't want to rewrite all keys every time we add a new entry, using a sliding window
    of indices (start, count) for the log entries. However, Preferences keys can only hold up to 16 characters, 
    which implies the key name will grow too long if the index increases. Let's re-sync the index starting from
    0 when it grows out of hand.
    */

    const size_t maxCount = 20;
    const size_t rolloverIndex = 999;

    LogEntry logEntry{m_currentOperation.startTime, m_currentOperation.type, m_currentOperation.mode, stallCount, aborted};

    m_prefs.begin("logs", false);
    size_t start = m_prefs.getUInt("start", 0);
    size_t count = m_prefs.getUInt("count", 0);

    if (start + count > rolloverIndex) {
        logInfo(lcEngine, "Re-syncing logs...");
        m_prefs.end();
        std::vector<LogEntry> entries = logEntries();
        m_prefs.begin("logs", false);
        m_prefs.clear();
        m_prefs.end();
        for (const LogEntry &entry : entries) {
            addLogEntry(entry.timestamp, entry.type, entry.mode, entry.stallCount, entry.aborted);
        }
        m_prefs.begin("logs", false);
        start = m_prefs.getUInt("start", 0);
        count = m_prefs.getUInt("count", 0);
    } 

    String baseKey = "log" + String(start + count) + "_";
    m_prefs.putUInt((baseKey + "time").c_str(), (uint32_t)timestamp);
    m_prefs.putUInt((baseKey + "type").c_str(), (uint32_t)type);
    m_prefs.putUInt((baseKey + "mode").c_str(), (uint32_t)mode);
    m_prefs.putInt((baseKey + "fill").c_str(), m_fillSensor->initialized() ? m_fillSensor->fillLevel() : -1);
    m_prefs.putInt((baseKey + "stalls").c_str(), stallCount);
    m_prefs.putBool((baseKey + "aborted").c_str(), aborted);

    if (count >= maxCount) {
        baseKey = "log" + String(start) + "_";
        m_prefs.remove((baseKey + "time").c_str());
        m_prefs.remove((baseKey + "type").c_str());
        m_prefs.remove((baseKey + "mode").c_str());
        m_prefs.remove((baseKey + "stalls").c_str());
        m_prefs.remove((baseKey + "aborted").c_str());
        m_prefs.putUInt("start", start+1);
        logInfo(lcEngine, "Added log entry at index " + String(start + count) + " and trimmed logs. Start: " + String(start) + ", Count: " + String(count));
    } else {
        m_prefs.putUInt("count", count + 1);
        logInfo(lcEngine, "Added log entry at index " + String(start + count) + ". Start: " + String(start) + ", Count: " + String(count));
    }
    m_prefs.end();
}
