cmake_minimum_required(VERSION 3.15)
project(amazingly_advanced)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Ofast")

find_package(SDL2 REQUIRED)
find_package(spdlog REQUIRED)
include_directories(amazingly_advanced ${SDL2_INCLUDE_DIRS})

add_executable(amazingly_advanced main.cpp src/utils/log.h src/gba.cpp src/gba.h src/mmu/mmu.cpp src/mmu/mmu.h src/utils/file_utils.h src/mmu/cartridge/cartridge.cpp src/mmu/cartridge/cartridge.h src/cpu/cpu.cpp src/cpu/cpu.h src/cpu/cpu_modes.h src/cpu/cpu_registers.h src/lcd/lcd.cpp src/lcd/lcd.h src/lcd/lcd_registers.h src/mmu/dma/dma.cpp src/mmu/dma/dma.h src/mmu/dma/dma_channels.h src/timer/timer.cpp src/timer/timer.h src/timer/timer_registers.h)
target_link_libraries(amazingly_advanced ${SDL2_LIBRARIES} spdlog::spdlog)