# Kromi One examples

Public code from [Kromi One](https://kromi.one), a new home computer for 2D and 2.5D games and creative work, in the tradition of the Amiga. Kromi One pairs the ARM cores of the AMD Kria K26 with its own chipset, Aura, which runs in the K26's programmable logic, and its own firmware, KROM. The developer edition is a prototype on the AMD Kria KV260 starter kit.

KROM's source code is closed while we develop it. This repository holds what we can share already: examples and tools for the KV260 that other developers can use, and later small example programs for Kromi One.

## What's here

- **[DDR4-2400 on the KV260](kv260-ddr4-2400/):** a Vivado script that lets the PS DDR4 run at the 2400 MT/s the board preset asks for, instead of 2133. MIT licence.
- **[Kromi One programs](programs/):** short programs in C, one idea each, written to be read: a program's start and the devices KROM offers it, a ball bouncing at 60 frames a second on the 2D chipset, and a textured card turning in perspective. They need the Kromi One SDK, which isn't public yet. MIT licence.

Each folder will state its licence.

## Follow the build

- Website and blog: https://kromi.one
- Hackster project: https://www.hackster.io/DarthIwan/kromi-one-a-new-home-computer-with-its-own-fpga-chipset-ef9f20
- Email: hello@kromi.one

---

AMD and Kria are trademarks or registered trademarks of Advanced Micro Devices, Inc. Kromi One is not affiliated with or endorsed by AMD.
