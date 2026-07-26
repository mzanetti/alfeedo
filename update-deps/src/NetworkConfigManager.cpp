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

#include "NetworkConfigManager.h"
#include "Logging.h"

#include <ESPmDNS.h>

LoggingCategory lcNetwork("Network");

const char* PreferencesPartition = "NetworkManager";
const char* PrefSSIDKey = "ssid";
const char* PrefPassKey = "pass";
const char* PrefModeKey = "mode";
const char* PrefHostnameKey = "hostname";

const char* DEFAULT_HOSTNAME = "Alfeedo";

const IPAddress AP_LOCAL_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);


void NetworkConfigManager::begin()
{
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info){
        onWifiEvent(event);
    });

    m_prefs.begin(PreferencesPartition, true); // Read-only
    m_ssid = m_prefs.getString(PrefSSIDKey, DEFAULT_HOSTNAME);
    m_pass = m_prefs.getString(PrefPassKey, "");
    m_hostname = m_prefs.getString(PrefHostnameKey, DEFAULT_HOSTNAME);
    m_mode = static_cast<WifiMode>(m_prefs.getInt(PrefModeKey, static_cast<int>(WifiMode::AP)));
    m_prefs.end();

    logInfo(lcNetwork, "Loaded WiFi credentials: SSID='" + m_ssid + "', Hostname='" + m_hostname + "', Mode='" + (m_mode == WifiMode::AP ? "AP" : "Client") + "'");

    
    configure(m_mode, m_ssid, m_pass);
    
    WiFi.setHostname(m_hostname.c_str());

    MDNS.begin(m_hostname.c_str());
}

void NetworkConfigManager::loop()
{
    // If in AP Mode, handle web requests and DNS queries
    if (m_state == NetworkState::Hotspot) {
        m_dnsServer.processNextRequest();
    }

    // Check the status and potentially retry connection
    checkWiFiStatus();
}

void NetworkConfigManager::configure(WifiMode mode, const String &ssid, const String &pass) {

    if (ssid.isEmpty()) {
        logWarning(lcNetwork, "SSID is empty. Cannot configure WiFi.");
        return;
    }

    m_mode = mode;
    m_ssid = ssid;
    m_pass = pass;

    m_prefs.begin(PreferencesPartition, false);
    m_prefs.putInt(PrefModeKey, static_cast<int>(mode));
    m_prefs.putString(PrefSSIDKey, ssid);
    m_prefs.putString(PrefPassKey, pass);
    m_prefs.end();

    logInfo(lcNetwork, "Saved WiFi credentials: SSID=" + m_ssid);

    if (mode == WifiMode::Client) {
        startClientMode();
    } else {
        startAPMode();
    }
}

void NetworkConfigManager::startAPMode(bool asFallback) {
    logInfo(lcNetwork, "Starting AP + Captive Portal Mode...");
        
    // Configure AP
    WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY, AP_SUBNET);
    String ssid;
    String pass;
    NetworkState newState;
    if (asFallback) {
        WiFi.mode(WIFI_MODE_APSTA); 
        ssid = DEFAULT_HOSTNAME;
        pass = "";
        newState = NetworkState::ConnectingWithHotspot;
    } else {
        WiFi.mode(WIFI_MODE_AP); 
        ssid = m_ssid;
        pass = m_pass;
        newState = NetworkState::Hotspot;
    }
    
    if (!WiFi.softAP(ssid.c_str(), pass.c_str())) {
      logError(lcNetwork, "AP failed to start!");
      return;
    }

    m_state = newState;
}

void NetworkConfigManager::startClientMode()
{
    logInfo(lcNetwork, "Attempting Client Mode...");
    m_state = NetworkState::Connecting;

    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(m_ssid.c_str(), m_pass.c_str());
    m_lastConnectionAttempt = millis();
}

String NetworkConfigManager::ssid() const {
    return m_ssid;
}

String NetworkConfigManager::pass() const {
    return m_pass;
}

void NetworkConfigManager::setHostname(const String &hostname) {
    m_hostname = hostname;
    m_prefs.begin(PreferencesPartition, false);
    m_prefs.putString("hostname", hostname);
    m_prefs.end();
    WiFi.setHostname(hostname.c_str());

    applyMDNSConfig();
}

String NetworkConfigManager::hostname() const {
    return m_hostname;
}

NetworkConfigManager::WifiMode NetworkConfigManager::wifiMode() const {
    return m_mode;
}

void NetworkConfigManager::reset() {
    logInfo(lcNetwork, "Resetting settings...");
    m_prefs.begin(PreferencesPartition, false);
    m_prefs.remove(PrefSSIDKey);
    m_prefs.remove(PrefPassKey);
    m_prefs.remove(PrefModeKey);
    m_prefs.remove(PrefHostnameKey);
    m_prefs.end();

    m_ssid = "";
    m_pass = "";
    m_hostname = "Alfeedo";
    m_mode = WifiMode::AP;

    logInfo(lcNetwork, "Cleared WiFi credentials.");

    startAPMode();
}

void NetworkConfigManager::onWifiEvent(WiFiEvent_t event)
{
    switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
        logDebug(lcNetwork, "WiFi interface ready");
        break;
    case ARDUINO_EVENT_WIFI_STA_START:
        logDebug(lcNetwork, "WiFi client connecting...");
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        logInfo(lcNetwork, "WiFi connected to AP");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        if (m_state != NetworkState::Connected) {
          logInfo(lcNetwork, "IP address obtained: " + WiFi.localIP().toString());
          m_state = NetworkState::Connected;
        }
        break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        if (m_state == NetworkState::Connected) {
            logInfo(lcNetwork, "Connection lost. Attempting to reconnect...");
            m_state = NetworkState::Connecting;
        } else if (m_state == NetworkState::Connecting) {
            logInfo(lcNetwork, "Connection attempt failed. Starting fallback AP mode while still trying to reconnect...");
            startAPMode(true);
        } else if (m_state == NetworkState::ConnectingWithHotspot) {
            logInfo(lcNetwork, "Connection attempt failed, Still trying to connect to WiFi...");
        }
        break;
    case ARDUINO_EVENT_WIFI_AP_START:
        logInfo(lcNetwork, "AP Mode started. SSID: " + WiFi.softAPSSID() + ", IP: " + WiFi.softAPIP().toString());
        m_dnsServer.start(53, "*", AP_LOCAL_IP);
        break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
        logInfo(lcNetwork, "AP Mode stopped.");
        m_dnsServer.stop();
        break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        logInfo(lcNetwork, "WiFi: A client connected to our AP");
        break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        logDebug(lcNetwork, "WiFi: A client disconnected from our AP");
        break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        logDebug(lcNetwork, "WiFi: Assigned an IP to the client");
        break;
    default:
        logDebug(lcNetwork, "[WiFi-Event] event: " + String(event));
        break;
    }
}

void NetworkConfigManager::checkWiFiStatus()
{
    unsigned long currentMillis = millis();

    if (m_state == NetworkState::Connecting || m_state == NetworkState::ConnectingWithHotspot || m_state == NetworkState::Connected) {
        if (m_state == NetworkState::Connected && WiFi.status() != WL_CONNECTED) {
            logWarning(lcNetwork, "WiFi connection lost.");
            return;
        }
      
        if ((m_state == NetworkState::Connecting || m_state == NetworkState::ConnectingWithHotspot) && (currentMillis - m_lastConnectionAttempt >= m_connectionAttemptInterval)) {
            logInfo(lcNetwork, "Retrying connection to " + m_ssid);
            WiFi.begin(m_ssid.c_str(), m_pass.c_str());
            m_lastConnectionAttempt = currentMillis;
        }

        if (m_state == NetworkState::Connected && WiFi.getMode() == WIFI_MODE_APSTA) {
            logInfo(lcNetwork, "Client connected. Shutting down AP mode.");
            WiFi.mode(WIFI_MODE_STA); 
        }
    }
}

void NetworkConfigManager::applyMDNSConfig() {
    MDNS.end();
    if (!MDNS.begin(m_hostname.c_str())) {
        logError(lcNetwork, "Error setting up mDNS!");
        return;
    }

    for (const auto& record : m_mdnsServices) {
        if (MDNS.addService(record.service.c_str(), "tcp", record.port)) {   
            for (const auto& txt : record.txtRecords) {
                MDNS.addServiceTxt(record.service.c_str(), "tcp", txt.first.c_str(), txt.second.c_str());
            }
            logInfo(lcNetwork, "mDNS service added: " + record.service + " on port " + String(record.port));
        } else {
            logError(lcNetwork, "Failed to add mDNS service: " + record.service);
        }
    }
}

void NetworkConfigManager::addMDNSService(const MDNSService &record) {
    m_mdnsServices.push_back(record);
    applyMDNSConfig();
}