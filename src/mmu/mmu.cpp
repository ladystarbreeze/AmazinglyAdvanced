#include "mmu.h"

#include "cartridge/cartridge.h"
#include "dma/dma.h"
#include "../gba.h"
#include "../lcd/lcd.h"
#include "dma/dma_channels.h"
#include "../timer/timer.h"

constexpr bool in_range(const uint32_t address, const uint32_t lower, const uint32_t upper)
{
    return (address >= lower) && (address < upper);
}

MMU::MMU(const char *const bios_path, const char *const rom_path, GBA *gba) :
wram_board(0x40000, 0), wram_chip(0x8000, 0), palette_ram(0x400, 0),
vram(0x18000, 0), oam(0x400, 0), gba(gba), interrupt_master_enable(0), interrupt_enable(0), interrupt_request_flags(0),
sound_bias(0)
{
    console = spdlog::stdout_color_mt("MMU");

    bios  = load_file(bios_path, true, 0x4000);
    cart  = std::make_unique<Cartridge>(rom_path);
    dma   = std::make_unique<DMA>(this);
    lcd   = std::make_unique<LCD>(this);
    timer = std::make_unique<Timer>(this);
}

MMU::~MMU()
= default;

uint8_t MMU::read8(const uint32_t address) const
{
    uint32_t addr_masked = address & 0x0FFFFFFFu;

    if (in_range(addr_masked, 0, 0x4000))
    {
        return bios[addr_masked];
    }
    else if (in_range(addr_masked, 0x2000000, 0x3000000))
    {
        return wram_board[addr_masked % 0x40000u];
    }
    else if (in_range(addr_masked, 0x3000000, 0x4000000))
    {
        return wram_chip[addr_masked % 0x8000u];
    }
    else if (in_range(addr_masked, 0x04000000, 0x4000800))
    {
        switch (addr_masked)
        {
            case 0x4000006:
                return lcd->regs.vcount & 0xFFu;
            default:
                console->warn("Unhandled read8 from IO port! Address: {:08X}h", addr_masked);
                return 0;
        }
    }
    else if (in_range(addr_masked, 0x6000000, 0x7000000))
    {
        return vram[addr_masked % 0x18000u];
    }
    else if (in_range(addr_masked, 0x08000000, 0x0E000000))
    {
        if ((addr_masked % 0x2000000) >= cart->cart_bounds)
        {
            console->error("Read out of bounds! Address: {:08X}h, ROM size: {:08X}h",
                           addr_masked, cart->cart_bounds);

            throw std::runtime_error("Read out of bounds!");
        }

        return cart->data[addr_masked % 0x2000000];
    }
    else if (addr_masked == 0xE000000)
    {
        //  64k = 0xC2
        // 128k = 0x62
        return 0x62;
    }
    else if (addr_masked == 0xE000001)
    {
        //  64k = 0x1C
        // 128k = 0x13
        return 0x13;
    }
    else if (in_range(addr_masked, 0xE000002, 0xE010000))
    {
        return 0;
    }

    console->error("Unhandled read8! Address: {:08X}h", addr_masked);

    return 0;

    throw std::runtime_error("Unhandled read8!");
}

uint16_t MMU::read16(const uint32_t address) const
{
    uint32_t addr_masked = address & 0x0FFFFFFFu;

    if (in_range(addr_masked, 0, 0x4000))
    {
        return *(uint16_t*)(bios.data() + addr_masked);
    }
    else if (in_range(addr_masked, 0x2000000, 0x3000000))
    {
        return *(uint16_t*)(wram_board.data() + (addr_masked % 0x40000u));
    }
    else if (in_range(addr_masked, 0x3000000, 0x4000000))
    {
        return *(uint16_t*)(wram_chip.data() + (addr_masked % 0x8000u));
    }
    else if (in_range(addr_masked, 0x04000000, 0x4000800))
    {
        switch (addr_masked)
        {
            case 0x4000000:
                return lcd->regs.dispcnt;
            case 0x4000004:
                return lcd->regs.dispstat;
            case 0x4000006:
                return lcd->regs.vcount;
            case 0x4000088:
                return sound_bias;
            case 0x4000100:
                return timer->get_counter(0);
            case 0x4000104:
                return timer->get_counter(1);
            case 0x4000108:
                return timer->get_counter(2);
            case 0x400010C:
                return timer->get_counter(3);
            case 0x4000128:
                return 0x80;
            case 0x4000130:
                return gba->get_input();
            case 0x4000200:
                return interrupt_enable;
            case 0x4000202:
                return interrupt_request_flags;
            case 0x4000208:
                return interrupt_master_enable;
            default:
                console->warn("Unhandled read16 from IO port! Address: {:08X}h", addr_masked);
                return 0;
        }
    }
    else if (in_range(addr_masked, 0x5000000, 0x6000000))
    {
        return *(uint16_t*)(palette_ram.data() + (addr_masked % 0x400u));
    }
    else if (in_range(addr_masked, 0x6000000, 0x7000000))
    {
        return *(uint16_t*)(vram.data() + (addr_masked % 0x18000u));
    }
    else if (in_range(addr_masked, 0x08000000, 0x0E000000))
    {
        if ((addr_masked % 0x2000000) >= cart->cart_bounds)
        {
            console->error("Read out of bounds! Address: {:08X}h, ROM size: {:08X}h",
                           addr_masked, cart->cart_bounds);

            return 0xFFFF;

            throw std::runtime_error("Read out of bounds!");
        }

        return *(uint16_t *)(cart->data.data() + (addr_masked % 0x2000000));
    }

    console->error("Unhandled read16! Address: {:08X}h", addr_masked);

    return 0;

    throw std::runtime_error("Unhandled read16!");
}

uint32_t MMU::read32(const uint32_t address) const
{
    uint32_t addr_masked = address & 0x0FFFFFFFu;

    if (in_range(addr_masked, 0, 0x4000))
    {
        return *(uint32_t*)(bios.data() + addr_masked);
    }
    else if (in_range(addr_masked, 0x2000000, 0x3000000))
    {
        return *(uint32_t*)(wram_board.data() + (addr_masked % 0x40000u));
    }
    else if (in_range(addr_masked, 0x3000000, 0x4000000))
    {
        return *(uint32_t*)(wram_chip.data() + (addr_masked % 0x8000u));
    }
    else if (in_range(addr_masked, 0x04000000, 0x4000800))
    {
        switch (addr_masked)
        {
            case 0x4000004:
                return lcd->regs.dispstat | (uint32_t)lcd->regs.vcount << 16u;
            case 0x40000DC:
                return (uint32_t)dma->channels[3].control.dmacnt_h << 16u;
            case 0x4000130:
                return gba->get_input() | 0xFFFF0000u;
            case 0x4000200:
                return interrupt_enable | (uint32_t)(interrupt_request_flags << 16u);
            default:
                console->warn("Unhandled read32 from IO port! Address: {:08X}h", addr_masked);
                return 0;
        }
    }
    else if (in_range(addr_masked, 0x5000000, 0x6000000))
    {
        return *(uint32_t*)(palette_ram.data() + (addr_masked % 0x400u));
    }
    else if (in_range(addr_masked, 0x6000000, 0x7000000))
    {
        return *(uint32_t*)(vram.data() + (addr_masked % 0x18000u));
    }
    else if (in_range(addr_masked, 0x7000000, 0x8000000))
    {
        return *(uint32_t*)(oam.data() + (addr_masked % 0x400u));
    }
    else if (in_range(addr_masked, 0x08000000, 0x0E000000))
    {
        if ((addr_masked % 0x2000000) >= cart->cart_bounds)
        {
            console->error("Read out of bounds! Address: {:08X}h, ROM size: {:08X}h",
                    addr_masked, cart->cart_bounds);

            throw std::runtime_error("Read out of bounds!");
        }

        return *(uint32_t *)(cart->data.data() + (addr_masked % 0x2000000));
    }

    console->error("Unhandled read32! Address: {:08X}h", addr_masked);

    return 0xC0FFEE;

    throw std::runtime_error("Unhandled read32!");
}

void MMU::write8(const uint8_t value, const uint32_t address)
{
    uint32_t addr_masked = address & 0x0FFFFFFFu;

    if (addr_masked < 0x4000)
    {
        return;
    }
    if (in_range(addr_masked, 0x2000000, 0x3000000))
    {
        wram_board[addr_masked % 0x40000u] = value;
        return;
    }
    else if (in_range(addr_masked, 0x3000000, 0x4000000))
    {
        wram_chip[addr_masked % 0x8000u] = value;
        return;
    }
    else if (in_range(addr_masked, 0x04000000, 0x4000800))
    {
        switch (addr_masked)
        {
            case 0x4000208:
                //console->info("Write to Interrupt Master Enable, Value: {:02X}h", value);

                interrupt_master_enable = value;

                /*if (((uint16_t)value & 1u) != 0)
                {
                    console->critical("IME: Interrupts enabled!");
                }
                else
                {
                    console->critical("IME: Interrupts disabled!");
                }*/
                break;
            default:
                //console->warn("Unhandled write8 to IO port! Address: {:08X}h, value: {:02X}h", addr_masked, value);
                break;
        }

        return;
    }
    else if (in_range(addr_masked, 0x08000000, 0x0E000000))
    {
        console->warn("Write to cartridge area, Address: {:08X}h, value: {:02X}h", addr_masked, value);
        return;
    }
    else if (in_range(addr_masked, 0xE000000, 0xE010000))
    {
        console->info("Write to FLASH, Address: {:08X}h, value: {:02X}h", addr_masked, value);
        return;
    }

    console->error("Unhandled write8! Address: {:08X}h, value: {:02X}h", addr_masked, value);

    throw std::runtime_error("Unhandled write8!");
}

void MMU::write16(const uint16_t value, const uint32_t address)
{
    uint32_t addr_masked = address & 0x0FFFFFFFu;

    if (in_range(addr_masked, 0, 0x4000))
    {
        return;
    }
    else if (in_range(addr_masked, 0x2000000, 0x3000000))
    {
        *(uint16_t*)(wram_board.data() + (addr_masked % 0x40000u)) = value;
        return;
    }
    else if (in_range(addr_masked, 0x3000000, 0x4000000))
    {
        *(uint16_t*)(wram_chip.data() + (addr_masked % 0x8000u)) = value;
        return;
    }
    else if (in_range(addr_masked, 0x04000000, 0x4000800))
    {
        switch (addr_masked)
        {
            case 0x4000000:
                console->info("Write to DISPCNT, Value: {:04X}h", value);

                lcd->regs.dispcnt = value;
                break;
            case 0x4000004:
                console->info("Write to DISPSTAT, Value: {:04X}h", value);

                lcd->regs.dispstat = (lcd->regs.dispstat & 0x0007u) | (value & 0xFFF8u);
                break;
            case 0x4000008:
                //console->info("Write to BG0CNT, Value: {:04X}h", value);

                lcd->regs.bg[0].bgcnt = value;
                break;
            case 0x400000A:
                lcd->regs.bg[1].bgcnt = value;
                break;
            case 0x400000C:
                lcd->regs.bg[2].bgcnt = value;
                break;
            case 0x400000E:
                lcd->regs.bg[3].bgcnt = value;
                break;
            case 0x4000010:
                lcd->regs.bg[0].bghofs = value;
                break;
            case 0x4000012:
                lcd->regs.bg[0].bgvofs = value;
                break;
            case 0x4000014:
                lcd->regs.bg[1].bghofs = value;
                break;
            case 0x4000016:
                lcd->regs.bg[1].bgvofs = value;
                break;
            case 0x4000018:
                lcd->regs.bg[2].bghofs = value;
                break;
            case 0x400001A:
                lcd->regs.bg[2].bgvofs = value;
                break;
            case 0x400001C:
                lcd->regs.bg[3].bghofs = value;
                break;
            case 0x400001E:
                lcd->regs.bg[3].bgvofs = value;
                break;
            case 0x4000088:
                sound_bias = value;
                break;
            case 0x40000DC:
                dma->set_count(3, value);
                break;
            case 0x4000100:
                timer->set_reload(0, value);
                break;
            case 0x4000102:
                timer->set_control(0, value);
                break;
            case 0x4000104:
                timer->set_reload(1, value);
                break;
            case 0x4000106:
                timer->set_control(1, value);
                break;
            case 0x4000108:
                timer->set_reload(2, value);
                break;
            case 0x400010A:
                timer->set_control(2, value);
                break;
            case 0x400010C:
                timer->set_reload(3, value);
                break;
            case 0x400010E:
                timer->set_control(3, value);
                break;
            case 0x4000200:
                console->info("Write to Interrupt Enable, Value: {:04X}h", value);

                interrupt_enable = value;
                break;
            case 0x4000202:
                console->info("Write to Interrupt Flags, Value: {:04X}h", value);

                interrupt_request_flags &= (uint16_t)~value;
                break;
            case 0x4000208:
                console->info("Write to Interrupt Master Enable, Value: {:04X}h", value);

                interrupt_master_enable = value;

                if (((uint16_t)value & 1u) != 0)
                {
                    console->critical("IME: Interrupts enabled!");
                }
                else
                {
                    console->critical("IME: Interrupts disabled!");
                }
                break;
            default:
                console->warn("Unhandled write16 to IO port! Address: {:08X}h, value: {:04X}h", addr_masked, value);
                break;
        }

        return;
    }
    else if (in_range(addr_masked, 0x5000000, 0x6000000))
    {
        //console->info("Write to palette RAM, Address: {:08X}h, value: {:04X}h", addr_masked, value);

        *(uint16_t*)(palette_ram.data() + (addr_masked % 0x400u)) = value;
        return;
    }
    else if (in_range(addr_masked, 0x6000000, 0x7000000))
    {
        *(uint16_t*)(vram.data() + (addr_masked % 0x18000u)) = value;
        return;
    }
    else if (in_range(addr_masked, 0x7000000, 0x8000000))
    {
        *(uint16_t*)(oam.data() + (addr_masked % 0x400u)) = value;
        return;
    }
    else if (in_range(addr_masked, 0x08000000, 0x0E000000))
    {
        console->warn("Write to cartridge area, Address: {:08X}h, value: {:04X}h", addr_masked, value);

        return;
    }

    console->error("Unhandled write16! Address: {:08X}h, value: {:04X}h", addr_masked, value);

    throw std::runtime_error("Unhandled write16!");
}

void MMU::write32(const uint32_t value, const uint32_t address)
{
    uint32_t addr_masked = address & 0x0FFFFFFFu;

    if (addr_masked < 0x4000)
    {
        return;
    }
    if (in_range(addr_masked, 0x2000000, 0x3000000))
    {
        *(uint32_t*)(wram_board.data() + (addr_masked % 0x40000u)) = value;
        return;
    }
    else if (in_range(addr_masked, 0x3000000, 0x4000000))
    {
        *(uint32_t*)(wram_chip.data() + (addr_masked % 0x8000u)) = value;
        return;
    }
    else if (in_range(addr_masked, 0x04000000, 0x4000800))
    {
        switch (addr_masked)
        {
            case 0x4000000:
                console->info("Write to DISPCNT, Value: {:04X}h", (uint16_t)value);

                lcd->regs.dispcnt = (uint16_t)value;
                break;
            case 0x40000D4:
                dma->set_s_addr(3, value);
                break;
            case 0x40000D8:
                dma->set_d_addr(3, value);
                break;
            case 0x40000DC:
                dma->set_count(3, value);
                dma->set_control(3, value >> 16u);
                break;
            case 0x4000100:
                timer->set_reload(0, value);
                timer->set_control(0, value >> 16u);
                break;
            case 0x4000104:
                timer->set_reload(1, value);
                timer->set_control(1, value >> 16u);
                break;
            case 0x4000108:
                timer->set_reload(2, value);
                timer->set_control(2, value >> 16u);
                break;
            case 0x400010C:
                timer->set_reload(3, value);
                timer->set_control(3, value >> 16u);
                break;
            case 0x4000200:
                console->info("Write to IE/IF, Value: {:08X}h", value);

                interrupt_enable = value;
                interrupt_request_flags &= (uint16_t)~(value >> 16u);
                break;
            case 0x4000208:
                console->info("Write to Interrupt Master Enable, Value: {:04X}h", (uint16_t)value);

                interrupt_master_enable = (uint16_t)value;

                if (((uint16_t)value & 1u) != 0)
                {
                    console->critical("IME: Interrupts enabled!");
                }
                else
                {
                    console->critical("IME: Interrupts disabled!");
                }
                break;
            default:
                console->warn("Unhandled write32 to IO port! Address: {:08X}h, value: {:08X}h", addr_masked, value);
                break;
        }
        return;
    }
    else if (in_range(addr_masked, 0x5000000, 0x6000000))
    {
        //console->info("Write to palette RAM, Address: {:08X}h, value: {:08X}h", addr_masked, value);

        *(uint32_t*)(palette_ram.data() + (addr_masked % 0x400u)) = value;
        return;
    }
    else if (in_range(addr_masked, 0x6000000, 0x7000000))
    {
        *(uint32_t*)(vram.data() + (addr_masked % 0x18000u)) = value;
        return;
    }
    else if (in_range(addr_masked, 0x7000000, 0x8000000))
    {
        *(uint32_t*)(oam.data() + (addr_masked % 0x400u)) = value;
        return;
    }
    else if (in_range(addr_masked, 0x08000000, 0x0E000000))
    {
        //console->warn("Write to cartridge area, Address: {:08X}h, value: {:08X}h", addr_masked, value);

        return;
    }

    return;

    console->error("Unhandled write32! Address: {:08X}h, value: {:08X}h", addr_masked, value);

    throw std::runtime_error("Unhandled write32!");
}
