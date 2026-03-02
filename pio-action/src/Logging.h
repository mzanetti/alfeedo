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

#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

#include <vector>

class LoggingCategory {
public:
    LoggingCategory(const String &category);

    String category() const;

private:
    String m_category;
};

void logInfo(const LoggingCategory &category, const String &message);
void logWarning(const LoggingCategory &category, const String &message);
void logError(const LoggingCategory &category, const String &message);
void logDebug(const LoggingCategory &category, const String &message);


class LogMessage {
public:
    enum class Level {
        None = 0,
        Debug = 1 << 0,
        Info = 1 << 1,
        Warning = 1 << 2,
        Error = 1 << 3
    };

    time_t timestamp() const;
    LoggingCategory category() const;
    Level level() const;
    String message() const;
    
private:
    friend class Logging;
    LogMessage(time_t timestamp, const LoggingCategory &category, const Level &level, const String &message);
    
    time_t m_timestamp;
    LoggingCategory m_category;
    Level m_level;
    String m_message;
};

inline LogMessage::Level operator|(LogMessage::Level a, LogMessage::Level b) {
    return static_cast<LogMessage::Level>(
        static_cast<int>(a) | static_cast<int>(b)
    );
}
inline bool operator&(LogMessage::Level a, LogMessage::Level b) {
    return static_cast<int>(a) & static_cast<int>(b);
}

class Logging {
public:
    static std::vector<LogMessage> logs();
    static void logMessage(const LoggingCategory &category, const LogMessage::Level level, const String &message);
    
    private:
    void appendLogMessage(const LoggingCategory &category, const LogMessage::Level level, const String &message);
    static Logging* s_instance;
    std::vector<LogMessage> m_logs;
    LogMessage::Level m_logLevels = LogMessage::Level::Debug | LogMessage::Level::Info | LogMessage::Level::Warning | LogMessage::Level::Error;

};


#endif // LOGGING_H