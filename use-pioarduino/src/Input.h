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

#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include <functional>


typedef std::function<void()> InputHandler;

class Input
{
public:
    void setup(uint8_t pin, InputHandler handler);
    void loop();

    void addButton(int pin, int initial_state = LOW);

private:
    InputHandler m_handler;
    uint8_t m_pin;
    uint8_t m_lastButtonState;
    unsigned long m_lastDebounceTime;
    uint8_t m_buttonState;

};

#endif // INPUT_H