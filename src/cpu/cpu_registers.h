#pragma once
#ifndef AMAZINGLY_ADVANCED_CPU_REGISTERS_H
#define AMAZINGLY_ADVANCED_CPU_REGISTERS_H


#include <cinttypes>

union CPSR
{
    struct
    {
        uint32_t cpu_mode : 5;
        bool thumb_state  : 1;
        bool fiq_disable  : 1;
        bool irq_disable  : 1;
        uint32_t reserved : 20;
        bool overflow     : 1;
        bool carry        : 1;
        bool zero         : 1;
        bool negative     : 1;
    } ;

    uint32_t cpsr;
};

struct CPU_Registers
{
    uint32_t r_unbanked[8];
    uint32_t r_banked_fiq[5][2];

    uint32_t sp_banked[6];
    uint32_t lr_banked[6];
    uint32_t pc;

    CPSR cpsr;
    CPSR spsr_banked[5];
};


#endif //AMAZINGLY_ADVANCED_CPU_REGISTERS_H
