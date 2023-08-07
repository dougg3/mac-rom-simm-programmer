# Nuvoton/CMSIS files

These files came from [Nuvoton's M251 BSP](https://github.com/OpenNuvoton/M251BSP/). They provide register defines, structs, startup code, the vector table, and linker scripts.

I've tried to avoid using Nuvoton's BSP driver code for various peripherals as much as possible, but I did use their USBD driver because it would be too difficult to reimplement from scratch.
