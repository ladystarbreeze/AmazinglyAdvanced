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
#ifndef AMAZINGLY_ADVANCED_CPU_MODES_H
#define AMAZINGLY_ADVANCED_CPU_MODES_H


#include <cinttypes>

enum CPU_MODE
{
    User       = 0x10,
    FIQ        = 0x11,
    IRQ        = 0x12,
    Supervisor = 0x13,
    Abort      = 0x17,
    Undefined  = 0x1B,
    System     = 0x1F
};


#endif //AMAZINGLY_ADVANCED_CPU_MODES_H
