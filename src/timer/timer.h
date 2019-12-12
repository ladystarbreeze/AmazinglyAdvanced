#pragma once
#ifndef AMAZINGLY_ADVANCED_TIMER_H
#define AMAZINGLY_ADVANCED_TIMER_H


#include "timer_registers.h"

#include <cstdio>

#include "../utils/log.h"

class MMU;

class Timer
{
private:
    MMU *mmu;

    Timers timers[4];

    std::shared_ptr<spdlog::logger> console;
public:
    explicit Timer(MMU *mmu);
    ~Timer();

    [[nodiscard]] uint16_t get_counter(size_t timer) const;

    void set_control(size_t timer, uint16_t value);
    void set_reload(size_t timer, uint16_t value);

    void run();
};


#endif //AMAZINGLY_ADVANCED_TIMER_H
