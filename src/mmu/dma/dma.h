#pragma once
#ifndef AMAZINGLY_ADVANCED_DMA_H
#define AMAZINGLY_ADVANCED_DMA_H


#include "dma_channels.h"

#include "../../utils/file_utils.h"

#include <cstdio>

class MMU;

class DMA
{
private:
    MMU *mmu;

    std::shared_ptr<spdlog::logger> console;
public:
    explicit DMA(MMU *mmu);
    ~DMA();

    DMA_Channels channels[4];

    [[nodiscard]] bool is_enabled() const;
    [[nodiscard]] bool is_running() const;

    void set_control(size_t channel, uint16_t value);
    void set_count(size_t channel, uint16_t count);
    void set_d_addr(size_t channel, uint32_t d_addr);
    void set_s_addr(size_t channel, uint32_t s_addr);

    void check_start_cond();
    void run();
};


#endif //AMAZINGLY_ADVANCED_DMA_H
