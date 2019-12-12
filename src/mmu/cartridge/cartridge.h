#pragma once
#ifndef AMAZINGLY_ADVANCED_CARTRIDGE_H
#define AMAZINGLY_ADVANCED_CARTRIDGE_H


#include "..//..//utils/file_utils.h"

class Cartridge
{
private:
public:
    explicit Cartridge(const char *rom_path);
    ~Cartridge();

    std::vector<uint8_t> data;

    size_t cart_bounds;
};


#endif //AMAZINGLY_ADVANCED_CARTRIDGE_H
