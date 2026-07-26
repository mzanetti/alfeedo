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

#ifndef FILLSENSOR_H
#define FILLSENSOR_H

#include <Arduino.h>
#include <Adafruit_VL53L0X.h>
#include <Preferences.h>

#include <vector>

class FillSensor {
public:
    void begin();
    void loop();

    bool initialized() const;
    
    int fillLevel() const;
    int measurement() const;
    int rawMeasurement() const;

    int emptyMeasurement() const;
    void setEmptyMeasurement(int emptyMeasurement);

    int fullMeasurement() const;
    void setFullMeasurement(int fullMeasurement);

    void reset();

private:
    Adafruit_VL53L0X m_lox;
    bool m_initialized = false;

    Preferences m_prefs;
    int m_fillLevel = 0;
    int m_emptyMeasurement = 0;
    int m_fullMeasurement = 0;
    
    unsigned long m_lastReading = 0;
    std::vector<int> m_history;
    int m_rawMeasurement = 0;
    int m_measurement = 0;
};

#endif // FILLSENSOR_H