#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include "mapio.h"
#define ESCAPE          0x001B
#define LEFT_ARROW      0x4B00
#define RIGHT_ARROW     0x4D00
#define UP_ARROW        0x4800
#define DOWN_ARROW      0x5000
#define SCREEN_WIDTH    40
#define SCREEN_HEIGHT   24
#define STATUS_LINE     SCREEN_HEIGHT
#define CENTER_X        (SCREEN_WIDTH/2)
#define CENTER_Y        (SCREEN_HEIGHT/2)
#define TILE_WIDTH      16
#define TILE_HEIGHT     16
struct tile_t
{
    unsigned char tile[TILE_WIDTH * TILE_HEIGHT / 2];
    unsigned char tileExp[TILE_WIDTH * TILE_HEIGHT];
};
unsigned char far *vidmem = (char far *)0xB8000000L;
int mapWidth, mapHeight;
int tileCount;
unsigned char far * far *tileMap;
struct tile_t far *tileSet;
void expandTile(struct tile_t far *tileptr)
{
    int row, col;
    for (row = 0; row < TILE_HEIGHT; row++)
    {
        for (col = 0; col < TILE_WIDTH; col += 2)
        {
            tileptr->tileExp[row * 16 + col]     = tileptr->tile[row * 8 + col / 2] & 0x0F;
            tileptr->tileExp[row * 16 + col + 1] = tileptr->tile[row * 8 + col / 2] >> 4;
        }
    }
}
void loadmap(void)
{
    int i;
    unsigned long mapDim;
    struct tile_t far *tileptr;

    tileCount = tilesetLoad("demo.set", (unsigned char far * *)&tileSet, sizeof(struct tile_t));
    mapDim    = tilemapLoad("demo.map", (unsigned char far *)tileSet, sizeof(struct tile_t), &tileMap);
    mapHeight = mapDim >> 16;
    mapWidth  = mapDim;
    tileptr   = tileSet;
    for (i = 0; i < tileCount; i++)
        expandTile(tileptr++);
}
unsigned short extgetch(void)
{
    unsigned short extch;

    extch = getch();
    if (!extch)
        extch = getch() << 8;
    return extch;
}
void txt40(void)
{
    union REGS regs;

    regs.x.ax = 0x0001;     /* 40x25 color alphanumerics mode */
    int86(0x10, &regs, &regs);
    outp(0x3D8, 0x08);      /* turn off blink attribute on CGA */
    regs.x.ax = 0x1003;
    regs.x.bx = 0x0000;
    int86(0x10, &regs, &regs); /* turn off blink via EGA/VGA BIOS */
    regs.x.ax = 0x0200;
    regs.x.bx = 0x0000;
    regs.x.dx = (25 << 8);
    int86(0x10, &regs, &regs); /* move cursor off-screen */
}
void txt80(void)
{
    union REGS regs;

    regs.x.ax = 0x0003; /* 80x25 alphanumerics mode */
    regs.x.bx = 0x0000;
    int86(0x10, &regs, &regs);
}
void plot(unsigned int x, unsigned int y, unsigned char color)
{
    unsigned int pixaddr = (y * 80) + (x | 1);
    vidmem[pixaddr] = (vidmem[pixaddr] & 0x0F) | color;
}
int tile(int x, int y, int s, int t, int width, int height)
{
    unsigned char tileChar, borderColor, far *tileptr;
    int ss, w;
    int pixaddr;

    pixaddr = y * 80 + (x << 1);
    if ((x <= CENTER_X) && ((x + width)  > CENTER_X)
     && (y <= CENTER_Y) && ((y + height) > CENTER_Y))
        borderColor = 0x0F;
    else
        borderColor = 0x0;
    if (s >= 0 && t >= 0 && (s >> 4) < mapWidth && (t >> 4) < mapHeight)
    {
        tileptr = ((struct tile_t far *)tileMap[(t >> 4) * mapWidth + (s >> 4)])->tileExp;
        tileChar = 0xB1;
    }
    else
    {
        tileptr = NULL;
        tileChar = 0xDB;
    }
    if (tileptr)
    {
        s &= 0x0F;
        t &= 0x0F;
        tileptr   += t * 16 + s;
        while (height--)
        {
            ss = s;
            for (w = 0; w < width; w++)
            {
                tileChar = 0x20; // space
                if (t == 0 || t == 15)
                    tileChar = 0xC4;
                if (ss == 0)
                {
                    tileChar = 0xB3;
                    if (t == 0)
                        tileChar = 0xDA;
                    else if (t == 15)
                        tileChar = 0xC0;
                }
                else if (ss == 15)
                {
                    tileChar = 0xB3;
                    if (t == 0)
                        tileChar = 0xBF;
                    else if (t == 15)
                        tileChar = 0xD9;
                }
                vidmem[pixaddr + (w << 1)]     = tileChar;
                vidmem[pixaddr + (w << 1) + 1] = (tileptr[w] << 4) | borderColor;
                ss++;
            }
            t++;
            pixaddr = pixaddr + 80;
            tileptr += 16;
        }
    }
    else
    {   // Emptry tile
        borderColor |= 0x80;
        while (height--)
        {
            for (w = 0; w < width; w++)
            {
                vidmem[pixaddr + (w << 1)]     = tileChar;
                vidmem[pixaddr + (w << 1) + 1] = borderColor;
            }
            pixaddr = pixaddr + 80;
        }

    }
}
void tileRow(int y, int s, int t, int height)
{
    int x;

    x = TILE_WIDTH - (s & (TILE_WIDTH-1));
    tile(0, y, s, t, x, height);
    s = (s + TILE_WIDTH) & ~(TILE_WIDTH-1);
    for (; SCREEN_WIDTH - x > TILE_WIDTH; x += TILE_WIDTH)
    {
        tile(x, y, s, t, TILE_WIDTH, height);
        s += TILE_WIDTH;
    }
    tile(x, y, s, t, SCREEN_WIDTH - x, height);
}
void tileScrn(int s, int t)
{
    int y;

    y = TILE_HEIGHT - (t & (TILE_HEIGHT-1));
    tileRow(0, s, t, y);
    t = (t + TILE_HEIGHT) & ~(TILE_HEIGHT-1);
    for (; SCREEN_HEIGHT - y > TILE_HEIGHT; y += TILE_HEIGHT)
    {
        tileRow(y, s, t, TILE_HEIGHT);
        t += TILE_HEIGHT;
    }
    tileRow(y, s, t, SCREEN_HEIGHT - y);
}
/*
 * Tile map editor
 */
int main(int argc, char **argv)
{
    int orgS, orgT, extS, extT, centerS, centerT;
    unsigned char far *centerTile, tilePix;
    char quit, cycle;

    loadmap();
    txt40();
    orgS = 0;
    orgT = 0;
    extS = mapWidth  << 4;
    extT = mapHeight << 4;
    quit = 0;
    do
    {
        tileScrn(orgS, orgT);
        centerS = orgS + CENTER_X;
        centerT = orgT + CENTER_Y;
        centerTile = ((struct tile_t far *)tileMap[(centerT >> 4) * mapWidth + (centerS >> 4)])->tileExp;
        if (centerTile)
            tilePix = centerTile[(centerT & 0x0F) * 16 + (centerS & 0x0F)] << 4;
        else
            tilePix = 0x00;
        vidmem[CENTER_Y * SCREEN_WIDTH * 2 + CENTER_X * 2] = 0xCE;
        while (!kbhit())
        {
            vidmem[CENTER_Y * SCREEN_WIDTH * 2 + CENTER_X * 2 + 1] = tilePix | (cycle++ & 0x0F);
        }
        switch(extgetch())
        {
            case UP_ARROW:
                if (centerT > 0)
                    orgT--;
                break;
            case DOWN_ARROW:
                if (centerT < extT - 1)
                    orgT++;
                break;
            case LEFT_ARROW:
                if (centerS > 0)
                    orgS--;
                break;
            case RIGHT_ARROW:
                if (centerS < extS - 1)
                    orgS++;
                break;
            case ESCAPE:
            case 'q':
            case 'Q':
                quit = 1;
                break;
        }
    } while (!quit);
    txt80();
    return 0;
}
