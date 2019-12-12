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