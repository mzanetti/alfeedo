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

#ifndef OTAMANAGER_H
#define OTAMANAGER_H

#include <Arduino.h>
#include <SPIFFS.h> // for File

class OTAManager {
public:
    OTAManager();
    ~OTAManager();

    bool beginUpdate(size_t firmwareSize);
    size_t write(const uint8_t *data, size_t len);
    bool endUpdate();
    void abort();

    bool updateInProgress() const;
    int updateProgress() const;

private:
    size_t m_written = 0;
    size_t m_totalSize = 0;
};

#endif // OTAMANAGER_H