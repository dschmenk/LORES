#include <dos.h>
#include <conio.h>
#include "lores.h"
#include "tiler.h"
/*
 * Fast edge fill routines
 */
void setStartAddr(int addr);
void cpyEdgeH(int addr, unsigned char *edge);
void cpyEdgeV(int addr, unsigned char *edge);
/*
 * Graphics routines for 160x100x16 color mode
 */
unsigned char far *vidmem = (unsigned char far *)0xB8000000L;
unsigned char edgeH[2][80], edgeV[100];
unsigned int orgAddr = 0;
unsigned int orgS = 0;
unsigned int orgT = 0;
unsigned int maxS, maxT;
unsigned int spanMap;
unsigned char **tileMap;

int tile(unsigned int x, unsigned int y, unsigned int s, unsigned int t, int width, int height, unsigned char *tile)
{
    unsigned int pixaddr;
    int w;

    pixaddr = (y * 160 + x + orgAddr) & 0x3FFF;
    tile   += t * 8 + (s >> 1);
    width >>= 1;
    while (height--)
    {
        for (w = 0; w < width; w++)
            vidmem[pixaddr + (w << 1)] = tile[w];
        pixaddr += 160;
        tile    += 8;
    }
}
void tileRow(unsigned int y, unsigned int s, unsigned int t, int height, unsigned char **tileptr)
{
    unsigned int x;

    tile(0, y, s, t, 16 - s, height, *tileptr++);
    for (x = 16 - s; x < 160 - 16 - 1; x += 16)
        tile(x, y, 0, t, 16, height, *tileptr++);
    tile(x, y, 0, t, 160 - x, height, *tileptr++);
}
void tileScrn(unsigned int s, unsigned int t, unsigned char **tileptr)
{
    unsigned int y;

    tileRow(0, s, t, 16 - t, tileptr);
    tileptr += spanMap;
    for (y = 16 - t; y < 100 - 16 - 1; y += 16)
    {
        tileRow(y, s, 0, 16, tileptr);
        tileptr += spanMap;
    }
    tileRow(y, s, 0, 100 - y, tileptr);
}
/*
 * Tile edge into memory buffer
 */
void tileMemH(int x, unsigned int s, unsigned int t, int width, unsigned char *tile)
{
    tile   += (t << 3) + (s >> 1);
    x     >>= 1;
    width >>= 1;
    while (width--)
    {
        edgeH[0][x + width] = tile[width];
        edgeH[1][x + width] = tile[8 + width];
    }
}
void tileMemV(unsigned int y, unsigned int s, unsigned int t, int height, unsigned char *tile)
{
    tile += (t << 3) + (s >> 1);
    while (height--)
    {
        edgeV[y++] = *tile;
        tile      += 8;
    }
}
void tileEdgeH(unsigned int s, unsigned int t, unsigned char **tileptr)
{
    unsigned int x;

    tileMemH(0, s, t, 16 - s, *tileptr++);
    for (x = 16 - s; x < 160 - 16 - 1; x += 16)
        tileMemH(x, 0, t, 16, *tileptr++);
    tileMemH(x, 0, t, 160 - x, *tileptr++);
}
void tileEdgeV(unsigned int s, unsigned int t, unsigned char **tileptr)
{
    unsigned int y;

    tileMemV(0, s, t, 16 - t, *tileptr);
    tileptr += spanMap;
    for (y = 16 - t; y < 100 - 16 - 1; y += 16)
    {
        tileMemV(y, s, 0, 16, *tileptr);
        tileptr += spanMap;
    }
    tileMemV(y, s, 0, 100 - y, *tileptr);
}
unsigned long tileScroll(int scrolldir)
{
    unsigned int haddr, vaddr;

    if (scrolldir & SCROLL_LEFT)
    {
        if (orgS < maxS)
        {
            orgS   += 2;
            orgAddr = (orgAddr + 2) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_LEFT | SCROLL_RIGHT);
    }
    else if (scrolldir & SCROLL_RIGHT)
    {
        if (orgS > 0)
        {
            orgS   -= 2;
            orgAddr = (orgAddr - 2) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_LEFT | SCROLL_RIGHT);
    }
    if (scrolldir & SCROLL_UP)
    {
        if (orgT < maxT)
        {
            orgT   += 2;
            orgAddr = (orgAddr + 320) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_UP | SCROLL_DOWN);
    }
    else if (scrolldir & SCROLL_DOWN)
    {
        if (orgT > 0)
        {
            orgT   -= 2;
            orgAddr = (orgAddr - 320) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_UP | SCROLL_DOWN);
    }
    /*
     * Pre-render edges based on scroll direction
     */
     if (scrolldir & SCROLL_DOWN)
    {
        /*
         * Fill in top edge
         */
        tileEdgeH(orgS & 0x0F, orgT & 0x0F, tileMap + (orgT >> 4) * spanMap + (orgS >> 4));
        haddr = orgAddr;
    }
    else if (scrolldir & SCROLL_UP)
    {
        /*
         * Fill in botom edge
         */
        tileEdgeH(orgS & 0x0F, (orgT + 98) & 0x0F, tileMap + ((orgT + 98) >> 4) * spanMap + (orgS >> 4));
        haddr = (orgAddr + 98 * 160) & 0x3FFF;
    }
    if (scrolldir & SCROLL_RIGHT)
    {
        /*
         * Fill in left edge
         */
        tileEdgeV(orgS & 0x0F, orgT & 0x0F, tileMap + (orgT >> 4) * spanMap + (orgS >> 4));
        vaddr = orgAddr;
    }
    else if (scrolldir & SCROLL_LEFT)
    {
        /*
         * Fill right edge
         */
        tileEdgeV((orgS + 158) & 0x0F, orgT & 0x0F, tileMap + (orgT >> 4) * spanMap + ((orgS + 158) >> 4));
        vaddr = (orgAddr + 158) & 0x3FFF;
    }
    /*
     * The following happens during VBlank
     */
    setStartAddr(orgAddr >> 1);
    /*
     * Fill in edges
     */
    if (scrolldir & (SCROLL_UP | SCROLL_DOWN))
        cpyEdgeH(haddr, edgeH[0]);
    if (scrolldir & (SCROLL_LEFT | SCROLL_RIGHT))
        cpyEdgeV(vaddr, edgeV);
    /*
     * Return updated origin as 32 bit value
     */
     return ((unsigned long)orgT << 16) | orgS;
}
void tileInit(unsigned int s, unsigned int t, unsigned int width, unsigned int height, unsigned char **map)
{
    tileMap = map;
    spanMap = width;
    maxS    = (width  << 4) - 162;
    maxT    = (height << 4) - 102;
    orgS    = s & 0xFFFE;
    orgT    = t & 0xFFFE;
    orgAddr = (orgT * 160 + (orgS | 1)) & 0x3FFF;
    setStartAddr(orgAddr >> 1);
    tileScrn(orgS & 0x0F, orgT & 0x0F, tileMap + (orgT >> 4) * spanMap + (orgS >> 4));
}
