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
#ifndef AMAZINGLY_ADVANCED_DMA_CHANNELS_H
#define AMAZINGLY_ADVANCED_DMA_CHANNELS_H


#include <cinttypes>

struct DMA_Channels
{
    uint32_t dmasad;
    uint32_t dmadad;

    union
    {
        struct
        {
            uint16_t dmacnt_l;

            union
            {
                struct
                {
                    uint16_t unused : 5;
                    uint16_t da_control : 2;
                    uint16_t sa_control : 2;
                    bool repeat : 1;
                    bool type : 1;
                    bool drq : 1;
                    uint8_t timing : 2;
                    bool irq : 1;
                    bool enable : 1;
                };

                uint16_t dmacnt_h;
            };
        } control;

        uint32_t dmacnt;
    };

    uint32_t s_addr;
    uint32_t d_addr;
    uint32_t count;

    bool is_running;
};


#endif //AMAZINGLY_ADVANCED_DMA_CHANNELS_H
