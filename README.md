# AmazinglyAdvanced GBA emulator

AmazinglyAdvanced is a free and open-source GBA emulator written in C++.

# Current version

**The latest released version is 0.1.0.**

The ARM7TDMI core is fully-functional. (Unfortunately, it is not bug-free)

The LCD emulation supports text mode 0 and bitmap modes 3 and 4. It does **not** support mosaic and affine transformation. Furthermore, sprite rendering is not implemented yet.

Neither **DMA sound channels** nor **PSG sound channels** are implemented. (Related issue: DMA1 and DMA2 are forcibly disabled)

Timers are fully-functional.

**VBLANK and HBLANK DMA timings** are not implemented yet.

**Note that AmazinglyAdvanced is nowhere near cycle-accurate!**

# To-Dos

## ARM7TDMI core
* Fix bugs
* Improve accuracy

## LCD
* Add text modes 1 and 2
* Add bitmap mode 5
* Implement sprite rendering (Related: switch to per-scanline renderer)
* Implement mosaic and affine transformation

## Sound
* Add DMA sound channels
* Add PSG sound channels

# How to run games with AmazinglyAdvanced

To run games with AmazinglyAdvanced, pass paths to a BIOS and game ROM image as command-line arguments.

# Keyboard controls
* **A** -> **V key**
* **B** -> **C key**
* **L** -> **D key**
* **R** -> **F key**
* **Up/Down/Left/Right** -> **Arrow keys**
* **Select** -> **Backspace**
* **Start** -> **Enter (Numpad!)**
