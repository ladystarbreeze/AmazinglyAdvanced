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
#ifndef AMAZINGLY_ADVANCED_CPU_REGISTERS_H
#define AMAZINGLY_ADVANCED_CPU_REGISTERS_H


#include <cinttypes>

union CPSR
{
    struct
    {
        uint32_t cpu_mode : 5;
        bool thumb_state  : 1;
        bool fiq_disable  : 1;
        bool irq_disable  : 1;
        uint32_t reserved : 20;
        bool overflow     : 1;
        bool carry        : 1;
        bool zero         : 1;
        bool negative     : 1;
    } ;

    uint32_t cpsr;
};

struct CPU_Registers
{
    uint32_t r_unbanked[8];
    uint32_t r_banked_fiq[5][2];

    uint32_t sp_banked[6];
    uint32_t lr_banked[6];
    uint32_t pc;

    CPSR cpsr;
    CPSR spsr_banked[5];
};


#endif //AMAZINGLY_ADVANCED_CPU_REGISTERS_H
