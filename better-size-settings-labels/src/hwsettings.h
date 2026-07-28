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

#ifndef HWSETTINGS_H
#define HWSETTINGS_H


#define DRIVER_ENABLE_PIN 25
#define DRIVER_DIRECTION_PIN 32
#define DRIVER_STEP_PIN 33
#define DRIVER_DIAG_PIN 26

#define UART_TX_PIN 17
#define UART_RX_PIN 16

#define FEED_BUTTON_PIN 4
#define NEXT_BUTTON_PIN 23
#define PREVIOUS_BUTTON_PIN 18

#define MOTOR_STEPS_PER_REVOLUTION 200 // 1,8° steps
#define MOTOR_MICROSTEPS 16
#define MOTOR_RMS_CURRENT 800 // mA

#endif
