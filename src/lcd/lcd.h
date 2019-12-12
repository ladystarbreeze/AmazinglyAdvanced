/*
 * AmazinglyAdvanced is a WIP GBA emulator.
 * Copyright (C) 2019  Lady Starbreeze (Michelle-Marie Schiller)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

#pragma once
#ifndef AMAZINGLY_ADVANCED_LCD_H
#define AMAZINGLY_ADVANCED_LCD_H


#include "lcd_registers.h"

#include <array>
#include <vector>

struct Pixel
{
    Pixel() :
    p_index(0), priority(0), sprite(false)
    {

    }

    Pixel (uint8_t p_index, uint8_t priority, bool sprite = false) :
            p_index(p_index), priority(priority), sprite(sprite)
    {

    }

    uint8_t p_index;
    uint8_t priority;
    bool sprite;
};

class MMU;

class LCD
{
private:
    MMU *mmu;

    uint16_t lcd_cycle;

    inline void tick();
    inline void draw_pixel(uint16_t x, uint16_t y, uint16_t color);

    std::array<void(LCD::*)(), 6> modes;

    inline Pixel mode_0_get_bg(size_t bg);

    inline void mode_0();
    inline void mode_3();
    inline void mode_4();
    inline void unknown_mode();
public:
    explicit LCD(MMU *mmu);
    ~LCD();

    LCD_Registers regs;

    std::vector<uint8_t> framebuffer;

    void run();
};


#endif //AMAZINGLY_ADVANCED_LCD_H
