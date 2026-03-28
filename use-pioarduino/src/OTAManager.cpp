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

#include "OTAManager.h"
#include "Logging.h"

#include <Update.h>
#include <Esp.h>
#include <SPIFFS.h>

LoggingCategory lcOTA("OTA");

struct FlashTaskParam {
    String path;
    OTAManager *otaManager;
};


OTAManager::OTAManager() = default;
OTAManager::~OTAManager() = default;

bool OTAManager::beginUpdate(size_t firmwareSize) {
    if (firmwareSize == 0) {
        logError(lcOTA, "Invalid firmware size");
        return false;
    }

    if (!Update.begin(firmwareSize, U_FLASH)) {
        logError(lcOTA, String("Update.begin failed: ") + String(Update.errorString()));
        return false;
    }
    m_totalSize = firmwareSize;
    m_written = 0;
    return true;
}

size_t OTAManager::write(const uint8_t *data, size_t len) {
    if (m_totalSize == 0) {
        logWarning(lcOTA, "Update not started");
        return 0;
    }
    size_t written = Update.write(const_cast<uint8_t*>(data), len);
    if (written != len) {
        // Update.write may return fewer bytes on error
        logError(lcOTA, String("Write failed: ") + String(Update.errorString()));
    }
    m_written += written;
    return written;
}

bool OTAManager::endUpdate() {
    if (!m_totalSize > 0) {
        logWarning(lcOTA, "No update in progress");
        return false;
    }

    m_totalSize = 0;

    bool ok = Update.end(true);
    if (!ok) {
        logError(lcOTA, String("Update.end failed: ") + String(Update.errorString()));
        return false;
    }
    return true;
}

void OTAManager::abort() {
    if (m_totalSize > 0) {
        Update.abort();
    }
    m_totalSize = 0;
    m_written = 0;
}

bool OTAManager::updateInProgress() const
{
    return m_totalSize > 0;
}

int OTAManager::updateProgress() const
{
    if (m_totalSize > 0) {
        return m_written * 100 / m_totalSize;
    }
    return 0;
}