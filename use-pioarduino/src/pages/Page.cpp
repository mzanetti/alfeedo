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

#include "Page.h"

Page::Page(const String &name):
    m_name(name)
{
}

String Page::name() const {
    return m_name;
}

void Page::handleInput(DisplayController *controller, Engine *engine, DisplayController::ButtonInput input) {
    switch (input) {
    case DisplayController::BUTTON_NEXT:
        controller->nextMenuPage();
        break;
    case DisplayController::BUTTON_PREVIOUS:
        controller->previousMenuPage();
        break;
    }
}

void Page::exit(DisplayController *controller) {
    controller->exitMenu();
}

void Page::drawCenteredText(Adafruit_GFX &display, int cycle, const String &line1Text, int line1Size, const String &line2Text, int line2Size) {
    drawMenu(display, cycle, -1, line1Text, line1Size, line2Text, line2Size);
}

void Page::drawMenu(Adafruit_GFX &display, int cycle, int selection, const String &line1Text, int line1Size, const String &line2Text, int line2Size)
{
    display.setTextSize(line1Size);
    drawScrollingLabel(display, cycle, line1Text, selection == 0, 0);

    display.setTextSize(line2Size);
    drawScrollingLabel(display, cycle, line2Text, selection == 1, line1Size * 8 + 5);
}

bool Page::needsTimedRefresh() const {
    uint16_t now = millis();
    if (now < m_lastRefresh || now - m_lastRefresh > 50) {
        m_lastRefresh = now;
        return true;
    }
    return false;
}

void Page::drawTitle(Adafruit_GFX &display, const String &text, const unsigned char *iconData)
{
    display.setTextWrap(false);
    display.setTextColor(WHITE);
    display.setTextSize(2);

    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    display.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);

    const int iconSize = 24;
    uint16_t textX = (display.width() - (textWidth + iconSize + 4)) / 2;

    display.setCursor(textX, (display.height() - textHeight) / 2);
    display.print(text);

    display.drawXBitmap(textX - iconSize - 4, (display.height() - iconSize) / 2, iconData, iconSize, iconSize, SSD1306_WHITE);
}

void Page::drawScrollingLabel(Adafruit_GFX &display, int cycle, const String &text, bool selected, int yOffset)
{
    display.setTextWrap(false);
 
    int16_t x1, y1;
    uint16_t w, h;

    uint16_t selectorWidth = 0;
    if (selected) {
        display.getTextBounds(">", 0, 0, &x1, &y1, &selectorWidth, &h);
        selectorWidth += 1; // one more pixel to have a border to the scrolling text
    };

    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    uint16_t maxLineWidth = display.width() - (selectorWidth * 2);
    uint16_t overflow = std::max(0, w - maxLineWidth);

    int lineX = 0;
    if (!overflow) {
        // center text
        lineX = (display.width() - w) / 2;
    } else {
        // Modify cycle so that in every loop we stand still for some while before scrolling
        const int standStillCycles = 10;
        const int overshootCycles = 10;
        int updatedCycle = max(0, ((cycle % (standStillCycles + overflow + overshootCycles)) - standStillCycles));
        // scroll text
        uint16_t xOffset = overflow > 0 ? updatedCycle % (overflow + overshootCycles) : 0;
        lineX = selectorWidth - xOffset;
    }

    display.setCursor(lineX, yOffset);
    display.print(text);

    if (selected) {
        display.fillRect(0, yOffset - 1, selectorWidth, h + 2, BLACK);
        display.setCursor(0, yOffset);
        display.print("<");
        display.fillRect(display.width() - selectorWidth + 1, yOffset - 1, selectorWidth, h + 2, BLACK);
        display.setCursor(display.width() - selectorWidth + 1, yOffset);
        display.print(">");
    }
};

void Page::drawBarControl(Adafruit_GFX &display, float value, float minValue, float maxValue, int yOffset)
{
    int16_t x, y;
    uint16_t selectorWidth, selectorHeight;
    display.getTextBounds(">", 0, 0, &x, &y, &selectorWidth, &selectorHeight);
    if (value > minValue) {
        display.setCursor(0, yOffset);
        display.print("<");
    }
    if (value < maxValue) {
        display.setCursor(display.width() - selectorWidth + 1, 8 + 5);
        display.print(">");
    }
    int maxWidth = display.width() - (selectorWidth + 1) * 2;
    int barWidth = (value - minValue) * maxWidth / (maxValue - minValue);
    barWidth = max(barWidth, 1);
    display.fillRect(selectorWidth + 1, yOffset, barWidth, selectorHeight, WHITE);
}