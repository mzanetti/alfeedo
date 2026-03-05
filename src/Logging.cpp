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

#include "Logging.h"
#include <unordered_map>

const std::unordered_map<LogMessage::Level, String> levelStrings = {
    {LogMessage::Level::Info, "INFO"},
    {LogMessage::Level::Warning, "WARNING"},
    {LogMessage::Level::Error, "ERROR"},
    {LogMessage::Level::Debug, "DEBUG"}
};

Logging* Logging::s_instance = nullptr;

LoggingCategory::LoggingCategory(const String &category)
    : m_category(category)
{
}

String LoggingCategory::category() const {
    return m_category;
}

std::vector<LogMessage> Logging::logs() {
    if (s_instance == nullptr) {
        s_instance = new Logging();
    }
    return s_instance->m_logs;
}

void Logging::logMessage(const LoggingCategory &category, const LogMessage::Level level, const String &message) {
    if (s_instance == nullptr) {
        s_instance = new Logging();
    }

    if ((s_instance->m_logLevels & level) == 0) {
        return;
    }

    Serial.println("[" + levelStrings.at(level) + "] " + category.category() + ": " + message);

    s_instance->appendLogMessage(category, level, message);
}

void Logging::appendLogMessage(const LoggingCategory &category, const LogMessage::Level level, const String &message) {
    time_t now;
    time(&now);
    tm timeInfo;
    localtime_r(&now, &timeInfo);

    if (s_instance->m_logs.size() >= 50) {
        s_instance->m_logs.erase(s_instance->m_logs.begin());
    }
    s_instance->m_logs.push_back(LogMessage(now, category, level, message));
}

LogMessage::LogMessage(const time_t timestamp, const LoggingCategory &category, const Level &level, const String &message):
    m_timestamp(timestamp),
    m_category(category),
    m_level(level),
    m_message(message)
{
}

time_t LogMessage::timestamp() const {
    return m_timestamp;
}

LoggingCategory LogMessage::category() const {
    return m_category;
}

LogMessage::Level LogMessage::level() const {
    return m_level;
}

String LogMessage::message() const {
    return m_message;
}

void logInfo(const LoggingCategory &category, const String &message) {
    Logging::logMessage(category, LogMessage::Level::Info, message);
}

void logWarning(const LoggingCategory &category, const String &message) {
    Logging::logMessage(category, LogMessage::Level::Warning, message);
}

void logError(const LoggingCategory &category, const String &message) {
    Logging::logMessage(category, LogMessage::Level::Error, message);
}

void logDebug(const LoggingCategory &category, const String &message) {
    Logging::logMessage(category, LogMessage::Level::Debug, message);
}
