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
