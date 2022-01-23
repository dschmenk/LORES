# CGA LoRes 160x100x16 hardware assisted scrolling library and demo

A simple proof-of-concept demonstrating hardware scrolling with a tile based virtual map and sprites.

video : https://youtu.be/rIbONSlyQeU

In order to build, MSC 5.1 and MASM 5.1 are used to create a real-mode DOS program.

    MASM /ml CGAOPS.ASM;
    CL /Ox TILEDEMO.C LORES.C TILER.C CGAOPS.OBJ

To build with software scrolling as part of demo, build as:

    CL /Ox /DSW_SCROLL TILEDEMO.C LORES.C TILER.C CGAOPS.OBJ

To build with CGA snow checking (the default):

    CL /Ox /DCPYBUF=_cpyBufSnow TILEDEMO.C LORES.C TILER.C CGAOPS.OBJ

To build without CGA snow checking:

    CL /Ox /DCPYBUF=_cpyBuf TILEDEMO.C LORES.C TILER.C CGAOPS.OBJ

To run on modern hardware, a DOS emulator such as DOSBox-X can be used. Note that DOSBox in it's current form has CGA emulation bugs (DOSBox-X works fine).
