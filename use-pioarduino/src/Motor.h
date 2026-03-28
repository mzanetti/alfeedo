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

#ifndef MOTOR_H
#define MOTOR_H

#include <FastAccelStepper.h>
#include <Arduino.h>
#include <TMCStepper.h>

#define R_SENSE             0.11
#define DRIVER_ADDRESS      0b00

struct StallGuardISRContext {
    FastAccelStepper* stepper;
    SemaphoreHandle_t stallFlag;
};

class Motor {
public:
  Motor(int stepPin, int dirPin, int enablePin, HardwareSerial &serialPort, int uartRxPin, int uartTxPin, int diagPin);
  ~Motor();

  void begin();

  void move(float revolutions);
  void setSpeed(float speed);
  float getSpeed() const;
  bool isMoving() const;
  int stallCount() const;

  void loop();
private:
  enum class State {
      Idle,
      Moving,
      Stalled,
      Reverse,
  };

  FastAccelStepperEngine m_engine;
  FastAccelStepper *m_stepper;
  TMC2209Stepper m_driver;
  HardwareSerial &m_serialPort;
  StallGuardISRContext m_stallGuardContext;
  
  State m_state = State::Idle;
  int m_targetPosition = 0;
  long m_stallCount = 0;

  int m_stepPin = 0;
  int m_directionPin = 0;
  int m_enablePin = 0;
  int m_uartRxPin = 0;
  int m_uartTxPin = 0;
  int m_diagPin = 0;

};

#endif // MOTOR_H