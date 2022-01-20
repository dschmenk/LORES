# CGA LoRes 160x100x16 hardware scrolling demo

A simple proof-of-concept demonstrating hardware scrolling with a tile based virtual map.

In order to build, MSC 5.1 and MASM 5.1 are used to create a real-mode DOS program.

    MASM /ml FILLEDGE.ASM;
    CL /Ox TILER.C FILLEDGE.OBJ

To run on modern hardware, a DOS emulator such as DOSBox-X can be used. Note that DOSBox in it's current form has CGA emulation bugs (DOSBox-X works fine).
