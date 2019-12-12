#pragma once
#ifndef AMAZINGLY_ADVANCED_TIMER_REGISTERS_H
#define AMAZINGLY_ADVANCED_TIMER_REGISTERS_H


#include <cinttypes>

struct Timers
{
    uint16_t tmcnt_l;

    union
    {
        struct
        {
            uint16_t prescaler_select : 2;
            bool count_up : 1;
            uint16_t unused_1 : 3;
            bool irq   : 1;
            bool start : 1;
            uint8_t unused_2  : 8;
        } control;

        uint16_t tmcnt_h;
    };

    uint16_t counter;
    uint16_t sub_counter;
    bool overflow;
};


#endif //AMAZINGLY_ADVANCED_TIMER_REGISTERS_H
