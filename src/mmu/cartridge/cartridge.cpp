#include "cartridge.h"

Cartridge::Cartridge(const char *const rom_path)
{
    data = load_file(rom_path);
    cart_bounds = data.size() - 2u;
}

Cartridge::~Cartridge()
= default;
