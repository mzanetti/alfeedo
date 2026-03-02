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

#include "FillSensor.h"
#include "Logging.h"

#include <numeric>

LoggingCategory lcFillSensor("FillSensor");

constexpr const char *PREFS_NAME = "fillSensor";
constexpr int defaultFullMeasurement = 20;
constexpr int defaultEmptyMeasurement = 100;

constexpr int maxHistory = 20;

void FillSensor::begin()
{
    logInfo(lcFillSensor, "Initializing VL53L0X sensor...");

    if (!Wire.begin()) {
        logError(lcFillSensor, "I2C initialization failed");
        return;
    }

    Wire.beginTransmission(VL53L0X_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        logError(lcFillSensor, "VL53L0X not found on I2C bus");
        return;
    }

    if (!m_lox.begin()) {
        logError(lcFillSensor, "Failed to boot VL53L0X");
        return;
    }
    m_initialized = true;
    logInfo(lcFillSensor, "VL53L0X started");

    m_prefs.begin(PREFS_NAME);
    m_emptyMeasurement = m_prefs.getInt("empty", defaultEmptyMeasurement);
    m_fullMeasurement = m_prefs.getInt("full", defaultFullMeasurement);
    m_prefs.end();

    logInfo(lcFillSensor, "Settings loaded. Full: " + String(m_fullMeasurement) + ", Empty: " + String(m_emptyMeasurement));
}

void FillSensor::loop()
{
    if (!m_initialized) {
        return;
    }

    
    if (millis() - m_lastReading < 1000) {
        return;
    }
    m_lastReading = millis();

    VL53L0X_RangingMeasurementData_t measure;
    VL53L0X_Error status = m_lox.rangingTest(&measure, false);

    if (status != VL53L0X_ERROR_NONE) {
        logError(lcFillSensor, "Error during ranging");
        return;
    }

    if (measure.RangeStatus == 4) {  // phase failures have incorrect data
        logWarning(lcFillSensor, "Measurement out of range");
        return;
    }

    m_rawMeasurement = measure.RangeMilliMeter;

    if (abs(m_rawMeasurement - m_measurement) > 10) {
        logDebug(lcFillSensor, "Resetting fill sensor filter.");
        m_history.clear();
    }

    m_history.push_back(m_rawMeasurement);
    if (m_history.size() > maxHistory) {
        m_history.erase(m_history.begin(), m_history.begin() + m_history.size() - 10);
    }
    m_measurement = round(std::accumulate(m_history.begin(), m_history.end(), 0) / m_history.size());
    
    // // Linear (for a square container)
    // m_fillLevel = map(measure.RangeMilliMeter, m_emptyMeasurement, m_fullMeasurement, 0, 100);
    // m_fillLevel = constrain(m_fillLevel, 0, 100);
    

    // proportional (for a rectangular funnel)
    const float height = m_emptyMeasurement - m_fullMeasurement;
    const float lengthTop = 150;
    const float widthTop = 100;
    const float lengthBottom = 45;
    const float widthBottom = 35;
    const float areaTop = lengthTop * widthTop;
    const float areaBottom = lengthBottom * widthBottom;
    
    const float volumeTotal = (height / 3.0) * (areaTop + areaBottom + sqrt(areaTop * areaBottom));
    
    const float heightFood = max(m_emptyMeasurement - m_measurement, 0);
    const float lengthFood = lengthBottom + (heightFood / height) * (lengthTop - lengthBottom);
    const float widthFood = widthBottom + (heightFood / height) * (widthTop - widthBottom);
    const float areaFood = lengthFood * widthFood;
    
    const float volumeFood = (heightFood / 3.0) * (areaFood + areaBottom + sqrt(areaFood * areaBottom));
    
    m_fillLevel = volumeFood / volumeTotal * 100.0;
    m_fillLevel = constrain(m_fillLevel, 0, 100);
    
#if DEBUG_FILLSENSOR
    logDebug(lcFillSensor, "Distance (mm): " + String(m_measurement) + " (raw: " + String(m_rawMeasurement) + ")" 
        + ", FillLevel: " + String(m_fillLevel) 
        + ", Vol. food: " + String(volumeFood)
        + ", Vol total: " + String(volumeTotal)
        + ", Height total: " + String(height) + " food: " + String(heightFood));
#endif
}

bool FillSensor::initialized() const {
    return m_initialized;
}

int FillSensor::fillLevel() const
{
    return m_fillLevel;
}

int FillSensor::measurement() const
{
    return m_measurement;
}

int FillSensor::rawMeasurement() const {
    return m_rawMeasurement;
}

int FillSensor::emptyMeasurement() const
{
    return m_emptyMeasurement;
}

void FillSensor::setEmptyMeasurement(int emptyMeasurement)
{
    logInfo(lcFillSensor, "Empty measurement set to: " + String(emptyMeasurement));
    m_emptyMeasurement = emptyMeasurement;
    m_prefs.begin(PREFS_NAME);
    m_prefs.putInt("empty", m_emptyMeasurement);
    m_prefs.end();
}

int FillSensor::fullMeasurement() const
{
    return m_fullMeasurement;
}

void FillSensor::setFullMeasurement(int fullMeasurement)
{
    logInfo(lcFillSensor, "Full measurement set to: " + String(fullMeasurement));
    m_fullMeasurement = fullMeasurement;
    m_prefs.begin(PREFS_NAME);
    m_prefs.putInt("full", m_fullMeasurement);
    m_prefs.end();
}

void FillSensor::reset()
{
    logInfo(lcFillSensor, "Resetting settings...");
    m_fullMeasurement = defaultFullMeasurement;
    m_emptyMeasurement = defaultEmptyMeasurement;

    m_prefs.begin(PREFS_NAME);
    m_prefs.clear();
    m_prefs.end();
}
