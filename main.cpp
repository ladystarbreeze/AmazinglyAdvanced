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

#include "src/gba.h"
#include "src/utils/log.h"

#include <memory>

int main(const int argc, const char *const *const argv)
{
    auto console = spdlog::stdout_color_mt("AmazinglyAdvanced");
    std::unique_ptr<GBA> gba;

    spdlog::set_pattern("[%n] [%l] %v");
    console->info("WIP GBA emulator");
    console->info("Version 0.1.0");

    if (argc < 2)
    {
        console->error("Please provide a path to a GBA BIOS ROM image!");

        return 1;
    }
    else if (argc < 3)
    {
        console->error("Please provide a path to a GBA ROM image!");

        return 2;
    }
    else
    {
        try
        {
            gba = std::make_unique<GBA>(argv[1], argv[2]);

            gba->run();
        }
        catch (const std::runtime_error &e)
        {
            throw e;
        }
    }

    return 0;
}