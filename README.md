# CGA LoRes 160x100 16 Color Scrolling Tile and Sprite Library

![SPRTDEMO](https://github.com/dschmenk/LORES/blob/main/sprtdemo_000.png "sprtdemo")

Real mode DOS library providing hardware assisted scrolling using a tile based virtual map and sprites.

Contents:

- [Overview](#overview)
- [Implementation](#implementation)
  - [Scrolling and Tile Maps](#scrolling-and-tile-maps)
  - [Sprites](#sprites)
  - [Performance](#performance)
  - [Alternate Usage](#alternate-usage)
  - [EGA/VGA Compatibility](#ega-and-vga-compatibility)
- [Profiling](#profiling)
- [Dealing with CGA snow](#dealing-with-cga-snow)
- [The API](#the-api)
  - [Basic LoRes Routines](#basic-lores-routines)
  - [Tile Map](#tile-map)
  - [Sprites](#sprites-1)
  - [View](#view)
  - [Asset File I/O](#file-io)
- [Building](#building)
- [Editors](./SRC/MAPEDIT/README.md)
- [Samples and Demos](#samples-and-demos)
  - [Video](https://youtu.be/rIbONSlyQeU)

## Overview

The IBM PC was initially offered with two video cards. One was a monochrome, text only, 80 column by 25 line display. The other video card was the Color Graphics Adapter: CGA. It supported text and graphics mode, up to 640x200 high resolution single color graphics mode. A 320x200 four color graphics mode was also supported. However, a third mode, a low resolution 160x100 with sixteen colors was possible but not supported by the BIOS ROM. This third mode was in fact a tweaked 80 column text mode, setting the character cell to only two pixels high. By setting the text character to an extended code that split the cell in to foreground and background halves, each half cell could be set to one of sixteen colors creating 160x100 pseudo graphics mode.

There are some great features of this mode that make it attractive as a game mode: high enough resolution to make interesting graphics screens using all 16 colors available to the CGA card on both composite color monitors and the more expensive RGB monitors. It was also low enough resolution that a poor 4.77 MHz 8088 could handle pushing pixels around at an interactive speed.

Unfortunately, there are some big caveats with this mode: being a tweaked 80 column text mode, it suffers from video "snow" when accessed during active video generation. The way around this is to wait until inactive video before accessing the CGA's memory. Doing so incurs a huge performance penalty as the 8088 must busy wait until such time it can access the memory without conflict. Some compatible CGA card manufacturers designed their cards to avoid this conflict and could access the video memory without generating video "snow", thus achieving higher performance.

Creating fast graphics routines using this low resolution (lores) mode presents a real challenge, especially when coupled with a 4.77 MHz 8088. The goal of this project is to build a library of routines and tools to help write games that can take advantage of this under-utilized graphics mode.

## Implementation

In order to get fast, interactive graphics on such a low end system will require clever coding tricks to update the bare minimum of pixels per screen. The main idea of this library is to use a feature of the 6845 CRTC chip that adjusts its scan out start address to any character in CGA memory. The standard CGA card contains 16K of RAM used for background, foreground and character information (in text mode). Interestingly, the CGA responds to a full 32K of address space in the 8088's memory map. The CRTC also wraps its video scan out at 16K. The lores graphics mode uses almost all of this 16K to display a single screen: 80 * 50 = 8000 characters. Two bytes per character (color and character code) takes up 16000 bytes of the 16384 bytes of CGA memory. This leaves just 384 bytes (= 384 pixels) left over. Combined, the CPU and CRTC can access data off the end of the 16K memory that wraps around to the beginning of CGA memory.

### Scrolling and Tile Maps

With perfectly timed updates to the CRTC start address and updates to the border pixels, a virtual image can be scrolled around with only a little bit of overhead. This library is structured around the concept of a large map built of 16x16 pixel tiles. The coordinates are 12.4 fixed point values referred to as S and T, or (S,T) as a coordinate pair. The 4 bit fractional point allows for sub-tile coordinates. The on-screen view initially sets the (S,T) origin, then passes in a scroll direction during updates. There are limitations on how far scrolling happens during the update. Horizontally, the S coordinate will be forced to an even number. The CRTC chip can only set the start address to a character boundary. As lores mode uses a single character to represent two pixels, this limits the origin to even horizontal pixels. There is no such limitation on vertical pixels. Horizontally, scrolling occurs two pixels at a time. Vertically, scrolling can happen one pixel at a time, or to be consistent with the horizontal limitation, two pixels can optionally be vertically scrolled at a time.

In order to avoid visible anomalies during scrolling, all the updates have to happen during inactive video. This is obviously a challenge. Initially, the code waited until the CRTC signaled vertical blanking before rendering the updates. However, this didn't give enough time to update the border pixels on a slow CPU. By co-opting the Programmable Interval Timer (PIT) to synchronize with the last active scan line, the entire inactive video time (back porch, front porch, and vertical blank) could be utilized to handle scrolling and a little bit of additional rendering tasks.

### Sprites

To take advantage of the additional time available during inactive video, a pseudo sprite implementation was written to coordinate rendering bitmaps with transparency over the tile maps and scrolling view. The library tries to update the minimum number of pixels per frame and tries to avoid full erase and redraw if possible. Moving sprites by only a few pixels at a time allows for a background border to be included with a sprite update to remove the previous sprite image during a single pass rendering. All sprite images are pre-rendered into memory buffers before scrolling so they can be quickly updated. Sprites can overlap, with higher indeces rendering above lower indeces.

### Performance

The library is implemented in a combination of C and 8086 assembly. Any routine that touches pixel data is implemented in assembly for performance reasons. The higher level logic is generally written in C, for ease of understanding and development. Output from the C compiler is verified to not be doing anything horrifically inefficient. The code was profiled to get it fast. There may be a few more cycles to extract from the routines, but nothing that will change the overall approach to the library's implementation. The library itself is a small model C library but all assets are far data pointers. Because there aren't many CPU cycles available between frames, incremental rendering and clever scheduling are the keys to creating a successful game.

### Alternate Usage

Although the library focusses on the ability to use the CRTC to assist in scrolling a virtual tile map, that isn't the only way to use the routines. It is quite easy to use the memory buffer routines to create an off-screen image and copy only those sections that change per frame. This could be useful when a large part of the screen image changes and a high frame rate isn't required.

### EGA and VGA compatibility

This library is focussed on the CGA. However, most computers are equipped with an EGA (rarely) or a VGA (most popular). The techniques used to scroll the screen and minimize the number of pixels needing updating won't work on the EGA/VGA for technical reasons. By implementing a compatible version of the API to completely redraw each frame in a back buffer, a visually identical result can be achieved. The caveat is that the amount of pixels needing to be updated per frame is much higher, so a fairly beefy CPU (as compared to the 8088) is required to get an equivalent frame rate.

## Profiling

Graphics in games is always time critical, even more so with this library. In order to figure out where time is spent in the library and application code, a nifty feature of the CGA is used: border colors. The CGA allows real time updates to the border color. By changing the color of the border in sections of code, a bar graph of sorts is generated in the screen border regions. Because all rendering is synchronized to the inactive video timing, the changing of the border colors start with the updating of the on-screen view. It is informative to see where CPU time is spent throughout the code and can help optimize or change algorithms to fit the desired frame rate.

## Dealing with CGA Snow

It is quite possible that all rendering tasks won't complete during inactive video, leading to snow if care isn't taken. The library will use routines that go as fast as possible during the view update, forgoing snow checks. Code will have to be written to complete rendering during inactive video generation. Code that renders to the video memory can be compiled to use versions of the routines that check for snow, or not, depending on a `#define CGA_SNOW`.

## The API

This library takes a tiered approach to provide low level access to the CGA up to a fully managed tile map and sprite environment. There isn't a requirement to use all the features of the API, but some API calls are necessary to provide baseline functionality.

### Basic LoRes Routines

The simplest functions set the mode and provide low level access to the graphics operations. Tiling and sprites need not even be enabled for these routines.

Set 80x25 text mode:

    void txt80(void);

Set 160x100 16 color mode. Returns a value passed to viewInit() to set the graphics adapter:

    unsigned int gr160(unsigned char fill, unsigned char border);

Plot a pixel:

    void plot(unsigned int x, unsigned int y, unsigned char color);

Draw horizontal line:

    void hlin(unsigned int x1, unsigned int x2, unsigned int y, unsigned char color);

Draw vertical line:

    void vlin(unsigned int x, unsigned int y1, unsigned int y2, unsigned char color);

Draw arbitrary line:

    void line(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned char color);

Draw solid rectangle:

    void rect(unsigned int x, unsigned int y, int width, int height, unsigned char color);

Draw character string using 8x8 CGA font

    void text(unsigned int x, unsigned int y, unsigned char color, char *string);

Set border color (actually a #define of an outp() call):

    void rasterBorder(unsigned char color);

Enable/disable video out (also #define of outp()):

    void rasterDisable(void);
    void rasterEnable(void);

### Low level tile map routines (rarely need to call directly)

Fill memory buffer with portion of tile map:

    void tileBuf(unsigned int s, unsigned int t, int widthBuf, int heightBuf, unsigned char far *buf);

Fill screen with current tile map view:

    void tileScrn(unsigned int s, unsigned int t);

Copy memory buffer on top of tile map with on-screen clipping:

    void cpyBuf(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf);

### Tile Map

Update a tile in the map (changed or animated tiles):

    void tileUpdate(unsigned i, unsigned j, unsigned char far *tileNew);

### Low level sprite routines (rarely need to call directly):

Render sprite into a buffer with transparency:

    void spriteBuf(int x, int y, int width, int height, unsigned char far *sprite, int span, unsigned char far *buf);

## Sprites

Enable/disable a sprite in the sprite table:

    void spriteEnable(int index, unsigned int s, unsigned int t, int width, int height, unsigned char far *sprite);
    void spriteDisable(int index);

Update the sprite image:

    void spriteUpdate(int index, unsigned char far *imageNew);

Set position of a sprite in the tile map. Returns clipped (s,t) of sprite:

    unsigned long spritePosition(int index, unsigned int s, unsigned int t);

## View

Views are the highest level API call and do a lot behind the scenes. They manage all the minutiae of updating the scrolling of the tile map and sprites.

Initialize the tile map and render the initial view:

    void viewInit(unsigned int adapter, unsigned int s, unsigned int t, unsigned int width, unsigned int height, unsigned char far * far *map);

Clean everything up (must call to unhook PIT interrupt):

    void viewExit(void);

Big daddy of them all. Coordinates tile, sprite and scrolling updates. Return (s,t) coordinate of screen origin:

    unsigned long viewRefresh(int scrolldir);

Global variable that increments every frame:

    extern unsigned int frameCount;

## File I/O

Load tile set:

    int tilesetLoad(char *filename, unsigned char far * *tileset, int sizeoftile);

Save tile set:

    int tilesetSave(char *filename, unsigned char far *tileset, int sizeoftile, int count);

Load tile map:

    unsigned long tilemapLoad(char *filename, unsigned char far *tileset, int sizeoftile, unsigned char far * far * *tilemap);

Save tile map:

    int tilemapSave(char *filename, unsigned char far *tileset, int sizeoftile, unsigned char far * far *tilemap, int width, int height);

Load sprite page:

    int spriteLoad(char *filename, unsigned char far * *spritepage, int *width, int *height);

Save sprite page:

    int spriteSave(char *filename, unsigned char far *spritepage, int width, int height, int count);


## Building

In order to build, MSC 5.1 and MASM 5.1 are used to create a real-mode DOS program. Borland C libraries and binaries can be built using Borland C++ 2.0 (3.1 has been tried, too) and MASM 5.1.
The build setup has been greatly expanded to create two versions of the libraries and four versions of TILEDEMO. It is very sophisticated - a DOS BATCH file: BUILD.BAT for MSC 5.1 and BUILDBC.BAT for Borland C++ 2.0 ;-)

MSC5.1 built libraries are:

- LORES.LIB: No profiling
- LRPROF.LIB: Border color profiling (on CGA)

Borland C++ 2.0 built libraries are:

- LORESBC.LIB: No profiling
- LRPROFBC.LIB: Border color profiling (on CGA)

Libraries are located in the LIB directory. Header files are in the INC directory.

When building your own code, set the define for use with the correct library:

For border color profiling:

    /DPROFILE : LRPROF.LIB or LRPROFBC.LIB

For CGA snow checking:

    /DCGA_SNOW  : LORES.LIB or LORESBC.LIB

For both:

    /DPROFILE /DCGA_SNOW  : LRPROF.LIB or LRPROFBC.LIB

## Samples and Demos

Two samples used to develop the library API are TILEDEMO.C and SPRTDEMO.C. TILEDEMO.EXE can be built using software or hardware scrolling:

    cl /Ox /I..\inc /DSW_SCROLL tiledemo.c ..\lib\lores.lib

for the software implementation.

    cl /Ox /I..\inc tiledemo.c ..\lib\lores.lib

for the hardware implementation. This is a testbed program, so it isn't very pretty. There are some #define options to change how each of the software and hardware versions will run.

There is a sample project file, SPRTDEMO.PRJ for building the SPRTDEMO.EXE sample with the Borland IDE.

A playable demo, [Maze Runner](SRC/MAZERUNR/README.md), is available in the SRC\\MAZERUNR directory. It is also builds four versions of the EXE for each option combination using the BUILD.BAT in the Maze Runner directory.

Another playable game, [RepelZ](SRC/REPELZ/readme.md) Is a complete 2D scrolling game with multiple simultaneous sprites, sophisticated input, and complex enemy A.I.

To run on modern hardware, a DOS emulator such as DOSBox-X can be used. Note that DOSBox in it's current relase has CGA emulation bugs (DOSBox-X works fine), however, using the Tandy machine options does seem to work.
