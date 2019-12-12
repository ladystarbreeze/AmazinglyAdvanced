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
#ifndef AMAZINGLY_ADVANCED_TIMER_H
#define AMAZINGLY_ADVANCED_TIMER_H


#include "timer_registers.h"

#include <cstdio>

#include "../utils/log.h"

class MMU;

class Timer
{
private:
    MMU *mmu;

    Timers timers[4];

    std::shared_ptr<spdlog::logger> console;
public:
    explicit Timer(MMU *mmu);
    ~Timer();

    [[nodiscard]] uint16_t get_counter(size_t timer) const;

    void set_control(size_t timer, uint16_t value);
    void set_reload(size_t timer, uint16_t value);

    void run();
};


#endif //AMAZINGLY_ADVANCED_TIMER_H
