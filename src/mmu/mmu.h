/*
 * AmazinglyAdvanced is a WIP GBA emulator.
 * Copyright (C) 2019  Lady Starbreeze (Michelle-Marie Schiller)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

#pragma once
#ifndef AMAZINGLY_ADVANCED_MMU_H
#define AMAZINGLY_ADVANCED_MMU_H


#include "../utils/file_utils.h"

#include <memory>
#include <vector>

class Cartridge;
class DMA;
class GBA;
class LCD;
class Timer;

class MMU
{
    friend DMA;
    friend GBA;
    friend LCD;
    friend Timer;
private:
    std::shared_ptr<spdlog::logger> console;

    std::unique_ptr<Cartridge> cart;
    std::unique_ptr<DMA>  dma;
    std::unique_ptr<LCD>  lcd;
    std::unique_ptr<Timer> timer;
    std::vector<uint8_t> bios;

    std::vector<uint8_t> wram_board;
    std::vector<uint8_t> wram_chip;
    std::vector<uint8_t> palette_ram;
    std::vector<uint8_t> vram;
    std::vector<uint8_t> oam;
public:
    MMU(const char *bios_path, const char *rom_path, GBA *gba);
    ~MMU();

    GBA *gba;

    uint16_t interrupt_master_enable;
    uint16_t interrupt_enable;
    uint16_t interrupt_request_flags;
    uint16_t sound_bias;

    [[nodiscard]] uint8_t   read8(uint32_t address) const;
    [[nodiscard]] uint16_t read16(uint32_t address) const;
    [[nodiscard]] uint32_t read32(uint32_t address) const;

    void  write8(uint8_t  value, uint32_t address);
    void write16(uint16_t value, uint32_t address);
    void write32(uint32_t value, uint32_t address);
};


#endif //AMAZINGLY_ADVANCED_MMU_H
