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

#include "TimeSource.h"
#include "Logging.h"

#include "esp_sntp.h"
#include <cstring>
#include <sys/time.h>

#define SAVE_INTERVAL 60
#define NTP_SERVER "pool.ntp.org"


LoggingCategory lcTimeSource("TimeSource");

bool s_ntpSynchronized = false;

const std::vector<TimeSource::TimeZoneEntry> s_timeZones = {
    // --- UTC ---
    {"UTC (Universal Time)", "UTC0"},

    // --- Europe/Africa ---
    {"Europe/London (GMT/BST)", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Europe/Paris (CET/CEST)", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Athens (EET/EEST)", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Africa/Johannesburg (SAST)", "GMT-2"},
    {"Africa/Cairo (EET)", "EET-2"}, // No DST

    // --- Asia/Middle East ---
    {"Middle East/Riyadh (AST)", "GMT-3"},
    {"Asia/Tehran (IRST/IRDT)", "IRST-3:30IRDT,M3.3.5/0:00,M9.3.5/0:00"}, // +3:30 offset
    {"Asia/Dubai (GST)", "GMT-4"},
    {"Asia/Karachi (PKT)", "GMT-5"},
    {"Asia/Kolkata (IST)", "GMT-5:30"},   // +5:30 offset
    {"Asia/Kathmandu (NPT)", "GMT-5:45"}, // +5:45 offset
    {"Asia/Shanghai (CST)", "GMT-8"},
    {"Asia/Tokyo (JST)", "GMT-9"},

    // --- Australia/Pacific ---
    {"Australia/Darwin (ACST)", "GMT-9:30"}, // +9:30 offset
    {"Australia/Sydney (AEST/AEDT)", "AET-10AEDT,M10.1.0,M4.1.0/3"},
    {"New Zealand (NZST/NZDT)", "NZT-12NZDT,M9.5.0,M4.1.0/3"},

    // --- The Americas ---
    {"Brazil/Rio de Janeiro", "GMT+3"},
    {"Argentina/Buenos Aires", "GMT+3"},
    {"Canada/Newfoundland", "NFT3:30NDT,M3.2.0,M11.1.0"}, // -3:30 offset
    {"US/Eastern (EST/EDT)", "EST5EDT,M3.2.0,M11.1.0"},
    {"US/Central (CST/CDT)", "CST6CDT,M3.2.0,M11.1.0"},
    {"US/Mountain (MST/MDT)", "MST7MDT,M3.2.0,M11.1.0"},
    {"US/Pacific (PST/PDT)", "PST8PDT,M3.2.0,M11.1.0"},
    {"US/Alaska (AKST/AKDT)", "AKST9AKDT,M3.2.0,M11.1.0"},
    {"US/Hawaii (HST)", "GMT+10"}};

const int TIMEZONE_COUNT = sizeof(s_timeZones) / sizeof(s_timeZones[0]);


void TimeSource::begin()
{
    // Resgistering a callback for NTP synchronization events
    sntp_set_time_sync_notification_cb([](struct timeval *tv) {
        struct tm timeinfo;
        localtime_r(&tv->tv_sec, &timeinfo);
        logInfo(lcTimeSource, "NTP time synchronized: " + String(asctime(&timeinfo)));
        s_ntpSynchronized = true;
    });

    m_prefs.begin("TimeSource", true);
    time_t savedTime = m_prefs.getULong("savedTime", 0);
    String tzString = m_prefs.getString("timeZone", "UTC0");
    m_prefs.end();

    // First, starting up with the last remembered time in case we fail to get NTP sync, so we have at least some time set (even if it's wrong)
    struct timeval tv = {.tv_sec = savedTime, .tv_usec = 0};
    settimeofday(&tv, NULL);


    auto it = std::find_if(s_timeZones.begin(), s_timeZones.end(),
                           [=](const TimeZoneEntry &entry) {
                               return std::strcmp(entry.tzString, tzString.c_str()) == 0;
                           });
    if (it != s_timeZones.end()) {
        m_timeZone = *it;
    } else {
        logWarning(lcTimeSource, "TimeZone string '" + tzString + "' not found in list. Falling back to UTC.");
        m_timeZone = {"UTC (Universal Time)", "UTC0"};
    }
    
#ifdef USE_NTP
    configTzTime(m_timeZone.tzString, NTP_SERVER);
#else
    applyTimeZone();
#endif

    // Print to confirm
    struct tm timeinfo;
    localtime_r(&savedTime, &timeinfo);
    logInfo(lcTimeSource, "Resumed from saved time: " + String(asctime(&timeinfo)) + " - " + String(m_timeZone.friendlyName) + " - " + String(m_timeZone.tzString));
}

void TimeSource::loop()
{
    saveCurrentTime();
}

void TimeSource::reset() {
    logInfo(lcTimeSource, "Resetting settings...");
    m_prefs.begin("TimeSource", false);
    m_prefs.clear();
    m_prefs.end();
}

bool TimeSource::ntpSynchronized() const {
    return s_ntpSynchronized;
}

struct tm TimeSource::currentDateTime() const {
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    return timeinfo;
}

String TimeSource::currentTimeString() const {
    struct tm currentDateTime = this->currentDateTime();
    char buffer[9]; // HH:MM:SS
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &currentDateTime);
    return String(buffer);
}

String TimeSource::currentDateTimeString() const {
    struct tm currentDateTime = this->currentDateTime();
    char buffer[20]; // YYYY-MM-DDTHH:MM:SS
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &currentDateTime);
    return String(buffer);
}

void TimeSource::setCurrentDateTime(struct tm localtime) {
    time_t utcSeconds = mktime(&localtime);
    struct timeval tv = { .tv_sec = utcSeconds, .tv_usec = 0 };
    settimeofday(&tv, nullptr);
    m_lastSavedTime = 0;
}

TimeSource::TimeZoneEntry TimeSource::timeZone() const
{
    return m_timeZone;
}

bool TimeSource::setTimeZone(const TimeZoneEntry &timeZone)
{
    m_timeZone = timeZone;
    m_prefs.begin("TimeSource");
    m_prefs.putString("timeZone", timeZone.tzString);
    m_prefs.end();

    applyTimeZone();
    return true;
}

bool TimeSource::setTimeZone(const String &tzString)
{
    auto it = std::find_if(s_timeZones.begin(), s_timeZones.end(),
                           [=](const TimeZoneEntry &entry) {
                               return std::strcmp(entry.tzString, tzString.c_str()) == 0;
                           });
    if (it == s_timeZones.end()) {
        logWarning(lcTimeSource, "TimeZone string '" + tzString + "' not found in list. Time zone not changed.");
        return false;
    }
    return setTimeZone(*it);
}

void TimeSource::saveCurrentTime()
{
    time_t now;
    time(&now);

    if (now - m_lastSavedTime < SAVE_INTERVAL) {
        return;
    }
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    logDebug(lcTimeSource, "Saving current time: " + String(asctime(&timeinfo)) + " NTP synced: " + String(ntpSynchronized()));
    m_prefs.begin("TimeSource");
    m_prefs.putULong("savedTime", now);
    m_prefs.end();
    m_lastSavedTime = now;
}

std::vector<TimeSource::TimeZoneEntry> TimeSource::timeZoneList()
{
    return s_timeZones;
}

void TimeSource::applyTimeZone()
{
    setenv("TZ", m_timeZone.tzString, 1);
    tzset();
    logInfo(lcTimeSource, String("Time zone applied: ") + m_timeZone.friendlyName + " (" + m_timeZone.tzString + ")");
}