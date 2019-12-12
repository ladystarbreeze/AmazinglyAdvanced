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
#ifndef AMAZINGLY_ADVANCED_DMA_H
#define AMAZINGLY_ADVANCED_DMA_H


#include "dma_channels.h"

#include "../../utils/file_utils.h"

#include <cstdio>

class MMU;

class DMA
{
private:
    MMU *mmu;

    std::shared_ptr<spdlog::logger> console;
public:
    explicit DMA(MMU *mmu);
    ~DMA();

    DMA_Channels channels[4];

    [[nodiscard]] bool is_enabled() const;
    [[nodiscard]] bool is_running() const;

    void set_control(size_t channel, uint16_t value);
    void set_count(size_t channel, uint16_t count);
    void set_d_addr(size_t channel, uint32_t d_addr);
    void set_s_addr(size_t channel, uint32_t s_addr);

    void check_start_cond();
    void run();
};


#endif //AMAZINGLY_ADVANCED_DMA_H
