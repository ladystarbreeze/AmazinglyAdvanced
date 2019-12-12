#pragma once
#ifndef AMAZINGLY_ADVANCED_CPU_MODES_H
#define AMAZINGLY_ADVANCED_CPU_MODES_H


#include <cinttypes>

enum CPU_MODE
{
    User       = 0x10,
    FIQ        = 0x11,
    IRQ        = 0x12,
    Supervisor = 0x13,
    Abort      = 0x17,
    Undefined  = 0x1B,
    System     = 0x1F
};


#endif //AMAZINGLY_ADVANCED_CPU_MODES_H
