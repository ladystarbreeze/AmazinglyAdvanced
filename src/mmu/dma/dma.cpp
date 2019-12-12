#include "dma.h"

#include "../mmu.h"
#include "dma_channels.h"

DMA::DMA(MMU *const mmu) :
mmu(mmu), channels()
{
    console = spdlog::stdout_color_mt("DMA");
}

DMA::~DMA()
= default;

bool DMA::is_enabled() const
{
    return channels[0].control.enable || channels[1].control.enable ||
           channels[2].control.enable || channels[3].control.enable;
}

bool DMA::is_running() const
{
    return channels[0].is_running || channels[1].is_running || channels[2].is_running || channels[3].is_running;
}

void DMA::set_control(const size_t channel, const uint16_t value)
{
    bool old_enable = channels[channel].control.enable;

    console->info("Write to DMA{}CNT_H, Value: {:04X}h", channel, value);

    channels[channel].control.dmacnt_h = value;

    if (!old_enable && channels[channel].control.enable)
    {
        channels[channel].s_addr = channels[channel].dmasad;
        channels[channel].d_addr = channels[channel].dmadad;
        channels[channel].count = ((channels[channel].control.dmacnt_l != 0) ?
                                    channels[channel].control.dmacnt_l : ((channel == 3) ? 0x10000 : 0x4000));
    }
}

void DMA::set_count(const size_t channel, const uint16_t count)
{
    console->info("Write to DMA{}CNT_L, Value: {:04X}h", channel, count);

    channels[channel].control.dmacnt_l = count;
}

void DMA::set_d_addr(const size_t channel, const uint32_t d_addr)
{
    console->info("Write to DMA{}DAD, Value: {:08X}h", channel, d_addr);

    channels[channel].dmadad = d_addr;
}

void DMA::set_s_addr(const size_t channel, const uint32_t s_addr)
{
    console->info("Write to DMA{}SAD, Value: {:08X}h", channel, s_addr);

    channels[channel].dmasad = s_addr;
}

void DMA::check_start_cond()
{
    for (auto & channel : channels)
    {
        if (channel.control.enable && !channel.is_running)
        {
            switch (channel.control.timing)
            {
                case 0:
                    channel.is_running = true;
                    break;
                case 1:
                case 2:
                case 3:
                default:
                    console->error("Unhandled DMA start condition!");

                    throw std::runtime_error("Unhandled DMA start condition");
            }
        }
    }
}

void DMA::run()
{
    for (size_t i = 0; i < 4; i++)
    {
        if (channels[i].is_running)
        {
            //console->info("DMA{} is running - Count: {:08X}h", i, channels[i].count);

            if (channels[i].control.type)
            {
                mmu->write32(mmu->read32(channels[i].s_addr), channels[i].d_addr);
            }
            else
            {
                mmu->write16(mmu->read16(channels[i].s_addr), channels[i].d_addr);
            }

            switch (channels[i].control.da_control)
            {
                case 0:
                case 3:
                    channels[i].d_addr += ((channels[i].control.type) ? 4u : 2u);
                    break;
                case 1:
                    channels[i].d_addr -= ((channels[i].control.type) ? 4u : 2u);
                    break;
                case 2:
                default:
                    break;
            }

            switch (channels[i].control.sa_control)
            {
                case 0:
                    channels[i].s_addr += ((channels[i].control.type) ? 4u : 2u);
                    break;
                case 1:
                    channels[i].s_addr -= ((channels[i].control.type) ? 4u : 2u);
                    break;
                case 2:
                case 3:
                default:
                    break;
            }

            --channels[i].count;

            if (channels[i].count == 0)
            {
                if (channels[i].control.repeat)
                {
                    channels[i].count = ((channels[i].control.dmacnt_l != 0) ?
                                         channels[i].control.dmacnt_l : ((i == 3) ? 0x10000 : 0x4000));

                    if (channels[i].control.da_control == 3)
                    {
                        channels[i].d_addr = channels[i].dmadad;
                    }
                }
                else
                {
                    channels[i].control.enable = false;
                }

                if (channels[i].control.irq)
                {
                    mmu->interrupt_request_flags |= (0x100u << i);
                }

                channels[i].is_running = false;
            }

            return;
        }
    }
}
