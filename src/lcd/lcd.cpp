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

#include "lcd.h"
#include "lcd_registers.h"

#include "../gba.h"
#include "../mmu/mmu.h"

const uint32_t SET_SIZE = 0x4000;
const uint32_t MAP_SIZE = 0x800;

const uint8_t map_width[]  = { 32, 64, 32, 64 };
const uint8_t map_height[] = { 32, 32, 64, 64 };

constexpr uint32_t get_map_offset(const uint32_t x, const uint32_t y)
{
    uint32_t offset = 0;

    if (x > 255)
    {
        ++offset;
    }
    if (y > 255)
    {
        ++offset;
    }
    if (x > 255 && y > 255)
    {
        ++offset;
    }

    return 0x800 * offset;
}

LCD::LCD(MMU *const mmu) :
mmu(mmu), lcd_cycle(0), modes(), regs(), framebuffer(240 * 160 * 2, 0)
{
    regs.control.forced_blank = true;

    for (size_t i = 0; i < 5; i++)
    {
        modes[i] = &LCD::unknown_mode;
    }

    modes[0] = &LCD::mode_0;
    modes[1] = &LCD::mode_0;
    modes[2] = &LCD::mode_0;
    modes[3] = &LCD::mode_3;
    modes[4] = &LCD::mode_4;
}

LCD::~LCD()
= default;

void LCD::tick()
{
    ++lcd_cycle;

    if (lcd_cycle == 240)
    {
        regs.status.hblank = true;

        if (regs.status.hblank_irq)
        {
            mmu->interrupt_request_flags |= 2u;
        }
    }
    else if (lcd_cycle == 308)
    {
        ++regs.vcount;
        lcd_cycle = 0;

        if (regs.vcount == regs.status.vcount_setting)
        {
            regs.status.vcount_coincidence = true;

            if (regs.status.vcount_coincidence_irq)
            {
                mmu->interrupt_request_flags |= 4u;
            }
        }
        else
        {
            regs.status.vcount_coincidence = false;
        }

        regs.status.hblank = false;
    }

    switch (regs.vcount)
    {
        case 160:
            if (lcd_cycle == 0)
            {
                regs.status.vblank = true;

                if (regs.status.vblank_irq)
                {
                    mmu->interrupt_request_flags |= 1u;
                }
            }
            break;
        case 227:
            if (lcd_cycle == 0)
            {
                regs.status.vblank = false;

                mmu->gba->draw_framebuffer(framebuffer.data());
            }
            break;
        case 228:
            regs.vcount = 0;
            break;
        default:
            break;
    }
}

void LCD::draw_pixel(const uint16_t x, const uint16_t y, const uint16_t color)
{
    uint32_t offset = (2u * x) + (2u * 240u * y);
    uint16_t color_swapped = ((color & 0x1Fu) << 10u) | (color & 0x3E0u) | ((color & 0x7C00u) >> 10u);

    *(uint16_t*)(framebuffer.data() + offset) = color_swapped;
}

Pixel LCD::mode_0_get_bg(const size_t bg)
{
    static uint8_t tile_offset[2] = { 32, 64 };

    uint32_t c_x = (regs.bg[bg].bghofs + regs.bg[bg].current_x) % (map_width[regs.bg[bg].control.bg_size] * 8u);
    uint32_t c_y = (regs.bg[bg].bgvofs + regs.vcount) % (map_height[regs.bg[bg].control.bg_size] * 8u);
    uint32_t map_offset = get_map_offset(c_x, c_y);

    if (c_x > 255)
    {
        c_x %= 256u;
    }
    if (c_y > 255)
    {
        c_y %= 256u;
    }

    uint32_t set_addr = 0x6000000u | (SET_SIZE * regs.bg[bg].control.character_base_block);
    uint32_t map_addr = 0x6000000u | (MAP_SIZE * regs.bg[bg].control.screen_base_block);

    uint32_t map_index = (uint32_t) (c_x / 8u) + 32 * (uint32_t)(c_y / 8u);

    uint16_t tile_index = mmu->read16(map_addr + map_offset + map_index * 2u);
    uint32_t tile_addr = set_addr + (tile_index & 0x3FFu) * tile_offset[(size_t)regs.bg[bg].control.color_mode];
    uint8_t palette_index;
    uint8_t priority = regs.bg[bg].control.priority;
    uint8_t p_index  = 0;

    if (regs.bg[bg].control.color_mode)
    {
        palette_index = mmu->read8(tile_addr + ((c_x ^ (7u * ((tile_index >> 10u) & 1u))) % 8u)
                                   + 8u * ((c_y ^ (7u * ((tile_index >> 11u) & 1u))) % 8u));
    }
    else
    {
        uint32_t n_tile_addr = tile_addr + ((c_x ^ (7u * ((tile_index >> 10u) & 1u))) % 8u) / 2u
                               + 4 * ((c_y ^ (7u * ((tile_index >> 11u) & 1u))) % 8u);
        p_index = mmu->read8(n_tile_addr);

        if (((tile_index >> 10u) & 1u) != 0)
        {
            p_index = (p_index << 4u) | (p_index >> 4u);
        }

        if ((c_x & 1u) == 1)
        {
            p_index >>= 4u;
        }

        p_index &= 0xFu;
        palette_index = 16u * (tile_index >> 12u) + p_index;
    }

    if ((regs.dispcnt & (0x100 << bg)) == 0)
    {
        palette_index = 0;
        priority = 4;
    }

    if (!regs.bg[bg].control.color_mode && p_index == 0)
    {
        palette_index = 0;
        priority = 4;
    }

    ++regs.bg[bg].current_x;

    return Pixel(palette_index, priority);
}

void LCD::mode_0()
{
    if (lcd_cycle >= 240 || regs.vcount >= 160)
    {
        for (auto &i : regs.bg)
        {
            i.current_x = 0;
        }

        return;
    }

    Pixel bg_pixels[4];
    Pixel final_pixel = Pixel(0, 4);

    for (size_t i = 0; i < 4; i++)
    {
        bg_pixels[i] = mode_0_get_bg(i);
    }

    for (size_t i = 4; i > 0; i--)
    {
        size_t index = i - 1u;

        if (bg_pixels[index].priority <= final_pixel.priority)
        {
            final_pixel.p_index  = bg_pixels[index].p_index;
            final_pixel.priority = bg_pixels[index].priority;
        }
    }

    if (regs.control.forced_blank)
    {
        draw_pixel(lcd_cycle, regs.vcount, 0xFFFF);
    }
    else
    {
        draw_pixel(lcd_cycle, regs.vcount, *(uint16_t*)(mmu->palette_ram.data() + (final_pixel.p_index * 2u) +
                ((final_pixel.sprite) ? 0x200 : 0)));
    }
}

void LCD::mode_3()
{
    if (lcd_cycle >= 240 || regs.vcount >= 160)
    {
        return;
    }

    if (regs.control.forced_blank)
    {
        draw_pixel(lcd_cycle, regs.vcount, 0xFFFF);
    }
    else
    {
        uint16_t color = *(uint16_t*)(mmu->vram.data() + ((lcd_cycle + (240u * regs.vcount)) * 2u));

        draw_pixel(lcd_cycle, regs.vcount, color);
    }
}

void LCD::mode_4()
{
    if (lcd_cycle >= 240 || regs.vcount >= 160)
    {
        return;
    }

    if (regs.control.forced_blank)
    {
        draw_pixel(lcd_cycle, regs.vcount, 0xFFFF);
    }
    else
    {
        uint8_t  palette_index = mmu->vram[lcd_cycle + (240u * regs.vcount)];
        uint16_t color = *(uint16_t*)(mmu->palette_ram.data() + (palette_index * 2u));

        draw_pixel(lcd_cycle, regs.vcount, color);
    }
}

void LCD::unknown_mode()
{
    if (regs.control.forced_blank)
    {
        return;
    }

    printf("BG mode: %u\n", regs.control.bg_mode);

    throw std::runtime_error("Unknown BG mode!");
}

void LCD::run()
{
    (this->*modes[regs.control.bg_mode])();
    tick();
}
