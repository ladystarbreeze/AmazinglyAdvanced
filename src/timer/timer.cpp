#include "timer.h"

#include "../mmu/mmu.h"
#include "timer_registers.h"

const uint16_t prescaler[4] = { 1, 64, 256, 1024 };

Timer::Timer(MMU *const mmu) :
mmu(mmu), timers()
{
    console = spdlog::stdout_color_mt("Timers");
}

Timer::~Timer()
= default;

uint16_t Timer::get_counter(const size_t timer) const
{
    return timers[timer].counter;
}

void Timer::set_control(const size_t timer, const uint16_t value)
{
    //console->info("Write to TM{}CNT_H, Value: {:04X}h", timer, value);

    bool old_start = timers[timer].control.start;

    timers[timer].tmcnt_h = value;
    timers[timer].sub_counter = 0;

    if (!old_start && timers[timer].control.start)
    {
        timers[timer].counter = timers[timer].tmcnt_l;
    }
}

void Timer::set_reload(const size_t timer, const uint16_t value)
{
    //console->info("Write to TM{}CNT_L, Value: {:04X}h", timer, value);

    timers[timer].tmcnt_l = value;
}

void Timer::run()
{
    for (size_t i = 0; i < 4; i++)
    {
        uint16_t old_c = timers[i].counter;

        timers[i].overflow = false;

        if (timers[i].control.start)
        {
            if (timers[i].control.count_up && i != 0)
            {
                if (timers[i - 1u].overflow)
                {
                    ++timers[i].counter;
                }
            }
            else
            {
                ++timers[i].sub_counter;

                if (timers[i].sub_counter >= prescaler[timers[i].control.prescaler_select])
                {
                    timers[i].sub_counter = 0;
                    ++timers[i].counter;
                }
            }

            if (old_c == 0xFFFF && timers[i].counter == 0)
            {
                //console->info("TM{} overflow", i);

                timers[i].overflow = true;
                timers[i].counter = timers[i].tmcnt_l;

                if (timers[i].control.irq)
                {
                    mmu->interrupt_request_flags |= (8u << i);
                }
            }
        }
    }
}
