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
#ifndef AMAZINGLY_ADVANCED_GBA_H
#define AMAZINGLY_ADVANCED_GBA_H


#include <memory>

#include <SDL2/SDL.h>

class CPU;
class MMU;

class GBA
{
private:
    std::shared_ptr<MMU> mmu;
    std::unique_ptr<CPU> cpu;

    SDL_Renderer *renderer;
    SDL_Window   *window;
    SDL_Texture  *texture;
    SDL_Event    event;

    bool is_running;

    void init_sdl();
public:
    GBA(const char *bios_path, const char *rom_path);
    ~GBA();

    uint16_t get_input();

    void draw_framebuffer(const uint8_t *framebuffer);
    void run();
};


#endif //AMAZINGLY_ADVANCED_GBA_H
