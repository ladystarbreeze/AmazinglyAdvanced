#pragma once
#ifndef AMAZINGLY_ADVANCED_LCD_H
#define AMAZINGLY_ADVANCED_LCD_H


#include "lcd_registers.h"

#include <array>
#include <vector>

struct Pixel
{
    Pixel() :
    p_index(0), priority(0), sprite(false)
    {

    }

    Pixel (uint8_t p_index, uint8_t priority, bool sprite = false) :
            p_index(p_index), priority(priority), sprite(sprite)
    {

    }

    uint8_t p_index;
    uint8_t priority;
    bool sprite;
};

class MMU;

class LCD
{
private:
    MMU *mmu;

    uint16_t lcd_cycle;

    inline void tick();
    inline void draw_pixel(uint16_t x, uint16_t y, uint16_t color);

    std::array<void(LCD::*)(), 6> modes;

    inline Pixel mode_0_get_bg(size_t bg);
    inline Pixel get_obj();

    inline void mode_0();
    inline void mode_3();
    inline void mode_4();
    inline void unknown_mode();
public:
    explicit LCD(MMU *mmu);
    ~LCD();

    LCD_Registers regs;

    std::vector<uint8_t> framebuffer;

    void run();
};


#endif //AMAZINGLY_ADVANCED_LCD_H
