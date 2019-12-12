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
#ifndef AMAZINGLY_ADVANCED_TIMER_REGISTERS_H
#define AMAZINGLY_ADVANCED_TIMER_REGISTERS_H


#include <cinttypes>

struct Timers
{
    uint16_t tmcnt_l;

    union
    {
        struct
        {
            uint16_t prescaler_select : 2;
            bool count_up : 1;
            uint16_t unused_1 : 3;
            bool irq   : 1;
            bool start : 1;
            uint8_t unused_2  : 8;
        } control;

        uint16_t tmcnt_h;
    };

    uint16_t counter;
    uint16_t sub_counter;
    bool overflow;
};


#endif //AMAZINGLY_ADVANCED_TIMER_REGISTERS_H
