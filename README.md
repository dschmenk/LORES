# CGA LoRes 160x100 16 color hardware assisted scrolling library and demo

A simple proof-of-concept demonstrating hardware scrolling with a tile based virtual map and sprites.

video : https://youtu.be/rIbONSlyQeU

In order to build, MSC 5.1 and MASM 5.1 are used to create a real-mode DOS program.
The build setup has been greatly expanded to create four versions of every binary/library.
Options are:

- LORES.LIB: fast, no snow checking or profiling
- LRPROF.LIB: fast with border color profiling
- LRSNOW.LIB: snow checking
- LRPROSNO.LIB: snow checking with border color profiling

A library for each build option is located in the LIB directory. Header files are now under the INC directory.

Two samples used to develop the library API are TILEDEMO.C and SPRTDEMO.C. TILEDEMO.EXE can be built using software or hardware scrolling:

    cl /Ox /I..\inc /DSW_SCROLL tiledemo.c ..\lib\lores.lib

for the software implementation.

    cl /Ox /I..\inc tiledemo.c ..\lib\lores.lib

for the hardware implementation. This is a testbed program, so it isn't very pretty. There are some #define options to change how each of the software and hardware versions will run.

A playable demo, Maze Runner, is available in the SRC\\MAZERUNR directory. It also build four versions of the EXE for each option combination.

To run on modern hardware, a DOS emulator such as DOSBox-X can be used. Note that DOSBox in it's current form has CGA emulation bugs (DOSBox-X works fine).
