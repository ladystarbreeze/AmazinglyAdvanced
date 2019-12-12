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
