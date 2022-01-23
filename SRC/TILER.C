#include <dos.h>
#include <conio.h>
#include "lores.h"
#include "tiler.h"
/*
 * Fast edge fill routines
 */
void setStartAddr(int addr);
void _cpyEdgeH(int addr);
void _cpyEdgeV(int addr);
#define CPYBUF
#ifdef CPYBUF
void _cpyBuf(int addr, int width, int height, int span, unsigned char *buf);
#endif
/*
 * Graphics routines for 160x100x16 color mode
 */
unsigned char far *vidmem = (unsigned char far *)0xB8000000L;
unsigned char edgeH[2][80], edgeV[100];
unsigned int orgAddr = 0;
unsigned int orgS = 0;
unsigned int orgT = 0;
unsigned int maxS, maxT, maxOrgS, maxOrgT;
unsigned int spanMap;
unsigned char **tileMap;

int tile(int x, int y, unsigned int s, unsigned int t, int width, int height, unsigned char *tileptr)
{
#ifdef CPYBUF
    _cpyBuf((y * 160 + x + orgAddr) & 0x3FFF, width >> 1, height, 8, tileptr + t * 8 + (s >> 1));
#else
    unsigned int pixaddr;
    int w;

    pixaddr = (y * 160 + x + orgAddr) & 0x3FFF;
    tile   += t * 8 + (s >> 1);
    width >>= 1;
    while (height--)
    {
        for (w = 0; w < width; w++)
            vidmem[pixaddr + (w << 1)] = tileptr[w];
        pixaddr += 160;
        tileptr += 8;
    }
#endif
}
void tileRow(int y, unsigned int s, unsigned int t, int height, unsigned char **tileptr)
{
    int x;

    x = 16 - s;
    tile(0, y, s, t, x, height, *tileptr++);
    while (x < 160 - 16)
    {
        tile(x, y, 0, t, 16, height, *tileptr++);
        x += 16;
    }
    tile(x, y, 0, t, 160 - x, height, *tileptr);
}
void tileScrn(unsigned int s, unsigned int t)
{
    int y;
    unsigned char **tileptr;

    tileptr = tileMap + (t >> 4) * spanMap + (s >> 4);
    s &= 0x0E;
    t &= 0x0E;
    y  = 16 - t;
    tileRow(0, s, t, y, tileptr);
    tileptr += spanMap;
    while (y < 100 - 16)
    {
        tileRow(y, s, 0, 16, tileptr);
        tileptr += spanMap;
        y += 16;
    }
    tileRow(y, s, 0, 100 - y, tileptr);
}
/*
 * Tile into memory buffer
 */
int tileMem(int x, int y, unsigned int s, unsigned int t, int width, int height, unsigned char *tile, int span, unsigned char *buf)
{
    int w;

    buf    += y * span + (x >> 1);
    tile   += t * 8 + (s >> 1);
    width >>= 1;
    while (height--)
    {
        for (w = 0; w < width; w++)
            buf[w] = tile[w];
        buf  += span;
        tile += 8;
    }
}
void tileBufRow(int y, unsigned int s, unsigned int t, int height, unsigned char **tileptr, int widthBuf, unsigned char *buf)
{
    int x, span;

    span = widthBuf >> 1;
    x    = 16 - s; // x is the width of the first tile and start of the second tile column
    if (x >= widthBuf)
        /*
         * Only one tile wide
         */
        tileMem(0, y, s, t, widthBuf, height, *tileptr, span, buf);
    else
    {
        /*
         * Two or more tiles wide
         */
        tileMem(0, y, s, t, x, height, *tileptr++, span, buf);
        while (x < widthBuf - 16)
        {
            tileMem(x, y, 0, t, 16, height, *tileptr++, span, buf);
            x += 16;
        }
        tileMem(x, y, 0, t, widthBuf - x, height, *tileptr, span, buf);
    }
}
void tileBuf(unsigned int s, unsigned int t, int widthBuf, int heightBuf, unsigned char *buf)
{
    int y;
    unsigned char **tileptr;

    tileptr = tileMap + (t >> 4) * spanMap + (s >> 4);
    s &= 0x0E;
    t &= 0x0E;
    y  = 16 - t; // y is the height of the first tile and start of second tile row
    if (y >= heightBuf)
        /*
         * Only one tile tall
         */
        tileBufRow(0, s, t, heightBuf, tileptr, widthBuf, buf);
    else
    {
        /*
         * Two or more tiles tall
         */
        tileBufRow(0, s, t, y, tileptr, widthBuf, buf);
        tileptr += spanMap;
        while (y < heightBuf - 16)
        {
            tileBufRow(y, s, 0, 16, tileptr, widthBuf, buf);
            tileptr += spanMap;
            y += 16;
        }
        tileBufRow(y, s, 0, heightBuf - y, tileptr, widthBuf, buf);
    }
}
/*
 * Copy buffer to screen
 */
void cpyBuf(unsigned int s, unsigned int t, int width, int height, unsigned char *buf)
{
    int w, span;
    unsigned int extS, extT;
    unsigned int pixaddr;

    /*
     * Calc screen extents
     */
    extS = orgS + 158;
    extT = orgT + 98;
    /*
     * Quick reject
     */
    if ((s > extS) || (s + width  <= orgS)
     || (t > extT) || (t + height <= orgT))
        return;
    /*
     * Clip to screen edges
     */
    span = width >> 1;
    if (s < orgS)
    {
        width -= orgS - s;
        buf   += (orgS - s) >> 1;
        s      = orgS;
    }
    else if (s + width > extS)
        width = extS - s;
    if (t < orgT)
    {
        height -= orgT - t;
        buf    += (orgT - t) * span;
        t       = orgT;
    }
    else if (t + height > extT)
        height = extT - t;
    /*
     * Copy to video memory
     */
#ifdef CPYBUF
    _cpyBuf(((t - orgT) * 160 + (s - orgS) + orgAddr) & 0x3FFF, width >> 1, height, span, buf);
#else
    pixaddr = ((t - orgT) * 160 + (s - orgS) + orgAddr) & 0x3FFF;
    width >>= 1;
     while (height--)
     {
         for (w = 0; w < width; w++)
             vidmem[pixaddr + (w << 1)] = buf[w];
         pixaddr += 160;
         buf     += span;
     }
#endif
}
/*
 * Tile into edge buffer
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
void tileMemV(int y, unsigned int s, unsigned int t, int height, unsigned char *tile)
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
    int x;

    tileMemH(0, s, t, 16 - s, *tileptr++);
    for (x = 16 - s; x < 160 - 16; x += 16)
        tileMemH(x, 0, t, 16, *tileptr++);
    tileMemH(x, 0, t, 160 - x, *tileptr++);
}
void tileEdgeV(unsigned int s, unsigned int t, unsigned char **tileptr)
{
    int y;

    tileMemV(0, s, t, 16 - t, *tileptr);
    tileptr += spanMap;
    for (y = 16 - t; y < 100 - 16; y += 16)
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
        if (orgS < maxOrgS)
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
        if (orgT < maxOrgT)
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
        tileEdgeH(orgS & 0x0E, orgT & 0x0E, tileMap + (orgT >> 4) * spanMap + (orgS >> 4));
        haddr = orgAddr;
    }
    else if (scrolldir & SCROLL_UP)
    {
        /*
         * Fill in botom edge
         */
        tileEdgeH(orgS & 0x0E, (orgT + 98) & 0x0E, tileMap + ((orgT + 98) >> 4) * spanMap + (orgS >> 4));
        haddr = (orgAddr + 98 * 160) & 0x3FFF;
    }
    if (scrolldir & SCROLL_RIGHT)
    {
        /*
         * Fill in left edge
         */
        tileEdgeV(orgS & 0x0E, orgT & 0x0E, tileMap + (orgT >> 4) * spanMap + (orgS >> 4));
        vaddr = orgAddr;
    }
    else if (scrolldir & SCROLL_LEFT)
    {
        /*
         * Fill right edge
         */
        tileEdgeV((orgS + 158) & 0x0E, orgT & 0x0E, tileMap + (orgT >> 4) * spanMap + ((orgS + 158) >> 4));
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
        _cpyEdgeH(haddr);
    if (scrolldir & (SCROLL_LEFT | SCROLL_RIGHT))
        _cpyEdgeV(vaddr);
    /*
     * Return updated origin as 32 bit value
     */
     return ((unsigned long)orgT << 16) | orgS;
}
void tileInit(unsigned int s, unsigned int t, unsigned int width, unsigned int height, unsigned char **map)
{
    tileMap = map;
    spanMap = width;
    maxS    = (width  << 4) - 2;
    maxT    = (height << 4) - 2;
    maxOrgS = maxS - 160;
    maxOrgT = maxT - 100;
    orgS    = s & 0xFFFE;
    orgT    = t & 0xFFFE;
    orgAddr = (orgT * 160 + orgS | 1) & 0x3FFF;
    setStartAddr(orgAddr >> 1);
    outp(0x3D8, 0x00);  /* Turn off video */
    tileScrn(orgS, orgT);
    outp(0x3D8, 0x09);  /* Turn on video */
}
