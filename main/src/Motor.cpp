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

#include "Motor.h"
#include "Logging.h"
#include "hwsettings.h"

LoggingCategory lcMotor("Motor");

HardwareSerial &TMC_SERIAL = Serial2;

Motor::Motor(int stepPin, int dirPin, int enablePin, HardwareSerial &serialPort, int uartRxPin, int uartTxPin, int diagPin):
    m_driver(&serialPort, R_SENSE, DRIVER_ADDRESS),
    m_serialPort(serialPort),
    m_stepPin(stepPin),
    m_directionPin(dirPin),
    m_enablePin(enablePin),
    m_uartRxPin(uartRxPin),
    m_uartTxPin(uartTxPin),
    m_diagPin(diagPin)
{
}

Motor::~Motor()
{
}

void IRAM_ATTR stallGuardInterrupt(void *arg)
{
    StallGuardISRContext *context = (StallGuardISRContext*)arg;

    int32_t currentPos = context->stepper->getCurrentPosition();
    context->stepper->forceStopAndNewPosition(currentPos);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(context->stallFlag, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }

    // Printing from ISR is generally not recommended, may cause watchdog resets if the ISR is triggered too often.
    // Serial.println("*** STALLGUARD at position: " + String(currentPos) + " ***");
}

void Motor::begin() {

    pinMode(m_enablePin, OUTPUT);
    pinMode(m_directionPin, OUTPUT);
    pinMode(m_stepPin, OUTPUT);

    m_engine.init();
    m_stepper = m_engine.stepperConnectToPin(m_stepPin);
    m_stepper->setDirectionPin(m_directionPin);
    m_stepper->setEnablePin(m_enablePin);
    m_stepper->setCurrentPosition(0);
    m_stepper->setAutoEnable(true);
    m_stepper->setAcceleration(10000);

    m_stallGuardContext.stallFlag = xSemaphoreCreateBinary();
    m_stallGuardContext.stepper = m_stepper;

    pinMode(m_diagPin, INPUT_PULLDOWN);
    if (digitalPinCanOutput(m_diagPin) < 0) {
        logError(lcMotor, "Stallguard pin not usable as interrupt pin");
    }
    attachInterruptArg(m_diagPin, stallGuardInterrupt, &m_stallGuardContext, RISING);

    m_serialPort.begin(115200, SERIAL_8N1, m_uartRxPin, m_uartTxPin);
    
    m_driver.begin();
    
    logInfo(lcMotor, "Connection to TMC2209: " + String(m_driver.test_connection() == 0 ? "Working" : "not working"));
    logDebug(lcMotor, "TMC2209 version: " + String(m_driver.version()));
    logDebug(lcMotor, "TMC2209 initial DRV_STATUS: " + String(m_driver.DRV_STATUS(), BIN));

    // There are a lot of incomplete resources on the TMC2209 out there
    // Most useful: https://gist.github.com/metalinspired/dcfe07ed0b9f42870eb54dcf8e29c126
    // Datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/TMC2209_datasheet_rev1.08.pdf

    m_driver.toff(4);
    m_driver.blank_time(32);
    m_driver.microsteps(MOTOR_MICROSTEPS);
    m_driver.stealth();
    m_driver.rms_current(MOTOR_RMS_CURRENT);
    m_driver.SGTHRS(5); // Stallguard threshold (sensitivity) Works fine with Bigtreetech v1.3
    // Minimum speed for stallguard to be enabled (0xFFFFF = enabled at all speeds)
    // When accellerating/decellerating it gets unreliable, so tuning it down
    m_driver.TCOOLTHRS(0x00300);  // Works fine with Bigtreetech 1.3
    
    
    logInfo(lcMotor, "TMC2209 configred. MS: " + String(m_driver.microsteps())
    + ", RMS Current: " + String(m_driver.rms_current()) + "mA"
    + ", SGTHRS: " + String(m_driver.SGTHRS())
    + ", TCOOLTHRS: " + String(m_driver.TCOOLTHRS())
    + ", TSTEP: " + String(m_driver.TSTEP())
    + ", GCONF: " + String(m_driver.GCONF(), BIN)
    );
    logDebug(lcMotor, "TMC2209 DRV_STATUS: " + String(m_driver.DRV_STATUS(), BIN));

}
 
void Motor::move(float revolutions)
{
    int steps = revolutions * MOTOR_STEPS_PER_REVOLUTION * (MOTOR_MICROSTEPS > 0 ? MOTOR_MICROSTEPS : 1);
    m_targetPosition = m_stepper->getCurrentPosition() + steps;
    m_stepper->moveTo(m_targetPosition);
    m_state = State::Moving;

    logInfo(lcMotor, "Moving motor " + String(revolutions) + " revs (" + String(steps) + " steps) to target " + String(m_targetPosition));
}   

void Motor::setSpeed(float speed)
{
    int microsteps = MOTOR_MICROSTEPS > 0 ? MOTOR_MICROSTEPS : 1;
    float stepsPerSec = speed * MOTOR_STEPS_PER_REVOLUTION * microsteps;
    logInfo(lcMotor, "Setting motor speed to " + String(speed) + " revolutions/sec (Steps/second: " + String(stepsPerSec) + ")");

    m_stepper->setSpeedInHz(speed * MOTOR_STEPS_PER_REVOLUTION * MOTOR_MICROSTEPS);
}

float Motor::getSpeed() const
{
    return 1.0 * const_cast<FastAccelStepper *>(m_stepper)->getSpeedInMilliHz() / 1000 / MOTOR_STEPS_PER_REVOLUTION / MOTOR_MICROSTEPS; 
}

void Motor::loop()
{
#if DEBUG_STALLGUARD
    static long last = 0;
    long now = millis();
    if (now - last > 500 && m_state != State::Idle) {
        last = now;
        int value = m_driver.SG_RESULT();
        // int tstep = m_driver.TSTEP();
        Serial.println("MOTOR_DBG: SG_RESULT: " + String(value));// + ", TSTEP: " + String(tstep));
    }
#endif

    if (xSemaphoreTake(m_stallGuardContext.stallFlag, 0) == pdTRUE && m_state == State::Moving) {
        logInfo(lcMotor, "Stall detected via StallGuard!");
        m_state = State::Stalled;
    }

    switch (m_state) {
    case State::Idle:
        break;
    case State::Moving:
        if (!m_stepper->isRunning()) {
            if (m_stepper->getCurrentPosition() == m_targetPosition){
                logInfo(lcMotor, "Target destination reached: " + String(m_stepper->getCurrentPosition()));
                m_stepper->setCurrentPosition(0);
                m_state = State::Idle;
            } else {
                logWarning(lcMotor, "Movement ended prematurely at position: " + String(m_stepper->getCurrentPosition()) + ", expected: " + String(m_targetPosition));
                m_state = State::Stalled;
            }
        }
        break;
    case State::Stalled: {
        logInfo(lcMotor, "Starting stallguard recovery routine (moving back 100 steps).");
        int32_t currentPos = m_stepper->getCurrentPosition();
        // m_stepper->forceStopAndNewPosition(currentPos);
        m_stepper->moveTo(currentPos-200);
        m_state = State::Reverse;
        m_stallCount++;
        break;
        }
    case State::Reverse:
        if (!m_stepper->isRunning()) {
            logInfo(lcMotor, "Resuming to target position: " + String(m_targetPosition));

            m_stepper->moveTo(m_targetPosition);
            m_state = State::Moving;
        }
        break;
    }
}

bool Motor::isMoving() const
{
    return m_state != State::Idle;
}

int Motor::stallCount() const {
    return m_stallCount;
}
