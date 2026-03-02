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

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <DNSServer.h>

class NetworkConfigManager
{
public:
    enum class WifiMode
    {
        Client,
        AP
    };
    enum class NetworkState
    {
        Init,
        Connecting,
        ConnectingWithHotspot,
        Connected,
        Hotspot
    };
    struct MDNSService
    {
        String service;
        int port;
        std::vector<std::pair<String, String>> txtRecords;
    };

    void begin();
    void loop();

    void configure(WifiMode mode, const String &ssid = "", const String &pass = "");
    WifiMode wifiMode() const;
    String ssid() const;
    String pass() const;

    void setHostname(const String &hostname);
    String hostname() const;

    void reset();
    
    void addMDNSService(const MDNSService &record);
    
private:
    void startClientMode();
    void startAPMode(bool asFallback = false);
    void onWifiEvent(WiFiEvent_t event);
    void checkWiFiStatus();

    void applyMDNSConfig();

    NetworkState m_state = NetworkState::Init;
    DNSServer m_dnsServer;
    Preferences m_prefs;
    String m_ssid;
    String m_pass;
    String m_hostname;
    WifiMode m_mode = WifiMode::AP;
    unsigned long m_lastConnectionAttempt = 0;
    const long m_connectionAttemptInterval = 10000; // 10 seconds
    std::vector<MDNSService> m_mdnsServices;
};

#endif