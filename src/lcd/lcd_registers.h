#pragma once
#ifndef AMAZINGLY_ADVANCED_LCD_REGISTERS_H
#define AMAZINGLY_ADVANCED_LCD_REGISTERS_H


#include <cinttypes>

struct BG
{
    union
    {
        struct
        {
            uint16_t priority : 2;
            uint16_t character_base_block : 2;
            uint16_t unused : 2;
            bool mosaic : 1;
            bool color_mode : 1;
            uint8_t screen_base_block : 5;
            bool wrap : 1;
            uint8_t bg_size : 2;
        } control;

        uint16_t bgcnt;
    };

    uint16_t bghofs;
    uint16_t bgvofs;

    uint8_t current_x;
};

struct LCD_Registers
{
    union
    {
        struct
        {
            uint16_t bg_mode   : 3;
            uint16_t reserved  : 1;
            bool frame_select  : 1;
            bool interval_free : 1;
            bool obj_vram_mapping : 1;
            bool forced_blank  : 1;
            bool bg0_enable : 1;
            bool bg1_enable : 1;
            bool bg2_enable : 1;
            bool bg3_enable : 1;
            bool obj_enable : 1;
            bool win0_enable : 1;
            bool win1_enable : 1;
            bool obj_win_enable : 1;
        } control;

        uint16_t dispcnt;
    };

    union
    {
        struct
        {
            uint16_t vblank : 1;
            bool hblank : 1;
            bool vcount_coincidence : 1;
            bool vblank_irq : 1;
            bool hblank_irq : 1;
            bool vcount_coincidence_irq : 1;
            uint8_t unused : 2;
            uint8_t vcount_setting : 8;
        } status;

        uint16_t dispstat;
    };

    uint16_t vcount;

    BG bg[4];
};


#endif //AMAZINGLY_ADVANCED_LCD_REGISTERS_H
