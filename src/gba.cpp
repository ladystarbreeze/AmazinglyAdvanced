#include "gba.h"

#include "cpu/cpu.h"
#include "lcd/lcd.h"
#include "mmu/mmu.h"
#include "mmu/dma/dma.h"
#include "timer/timer.h"

GBA::GBA(const char *const bios_path, const char *const rom_path) :
renderer(nullptr), window(nullptr), texture(nullptr), event(), is_running(true)
{
    mmu = std::make_shared<MMU>(bios_path, rom_path, this);
    cpu = std::make_unique<CPU>(mmu);

    init_sdl();
}

GBA::~GBA()
= default;

void GBA::init_sdl()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_CreateWindowAndRenderer(480, 320, 0, &window, &renderer);
    SDL_SetWindowSize(window, 480, 320); // 480, 320
    SDL_RenderSetLogicalSize(renderer, 480, 320);
    SDL_SetWindowResizable(window, SDL_FALSE);
    SDL_SetWindowTitle(window, "AmazinglyAdvanced v0.1.0");

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB555, SDL_TEXTUREACCESS_STREAMING, 240, 160);
}

uint16_t GBA::get_input()
{
    const uint8_t *keyboard_state = SDL_GetKeyboardState(nullptr);
    uint16_t input = 0;

    SDL_PollEvent(&event);

    if (event.type == SDL_KEYDOWN)
    {
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_x)])
        {
            input |= 1u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_y)])
        {
            input |= 2u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_BACKSPACE)])
        {
            input |= 4u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_KP_ENTER)])
        {
            input |= 8u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_RIGHT)])
        {
            input |= 0x10u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_LEFT)])
        {
            input |= 0x20u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_UP)])
        {
            input |= 0x40u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_DOWN)])
        {
            input |= 0x80u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_s)])
        {
            input |= 0x100u;
        }
        if (keyboard_state[SDL_GetScancodeFromKey(SDLK_a)])
        {
            input |= 0x200u;
        }
    }

    return ~input;
}

void GBA::draw_framebuffer(const uint8_t *framebuffer)
{
    SDL_UpdateTexture(texture, nullptr, framebuffer, 240 * sizeof(uint8_t) * 2);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void GBA::run()
{
    while (is_running)
    {
        try
        {
            if (mmu->dma->is_enabled())
            {
                mmu->dma->check_start_cond();
            }

            if (mmu->dma->is_running())
            {
                mmu->dma->run();
                mmu->dma->run();
            }
            else
            {
                cpu->run();
                cpu->run();
            }

            mmu->timer->run();
            mmu->timer->run();
            mmu->timer->run();
            mmu->timer->run();

            mmu->lcd->run();
        }
        catch (const std::runtime_error &e)
        {
            is_running = false;

            throw e;
        }
    }
}
