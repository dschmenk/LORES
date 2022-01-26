#include <dos.h>
#include <conio.h>
#include "lores.h"
#include "tiler.h"
extern unsigned int scanline[100]; // Precalculated scanline offsets
extern volatile unsigned int frameCount;
extern volatile unsigned char rasterTimer;
int enableRasterTimer(int scanline);
int disableRasterTimer(void);
int statusRasterTimer();
/*
 * Fast CGA routines
 */
void setStartAddr(int addr);
void _cpyEdgeH(int addr, int count);
void _cpyEdgeV(int addr);
void _cpyBuf(int addr, int width, int height, int span, unsigned char far *buf);
void _cpyBufSnow(int addr, int width, int height, int span, unsigned char far *buf);
#ifndef CPYBUF
#define CPYBUF  _cpyBuf
#endif
#define tile(x,y,s,t,w,h,p) CPYBUF((scanline[y]+(x)+orgAddr)&0x3FFF,(w)>>1,h,8,(p)+(t)*8+((s)>>1))
/*
 * Fast memory routines
 */
void tileMem(int x, int y, unsigned int s, unsigned int t, int width, int height, unsigned char far *tile, int span, unsigned char far *buf);
void tileEdgeH(unsigned int s, unsigned int t, unsigned char far * far*tileptr);
void tileEdgeH2(unsigned int s, unsigned int t, unsigned char far * far*tileptr);
void tileEdgeV(unsigned int s, unsigned int t, unsigned char far * far *tileptr);
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
unsigned char far * far *tileMap;

void tileRow(int y, unsigned int s, unsigned int t, int height, unsigned char far * far *tileptr)
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
    unsigned char far * far *tileptr;

    tileptr = tileMap + (t >> 4) * spanMap + (s >> 4);
    s &= 0x0F;
    t &= 0x0F;
    y  = 16 - t;
    tileRow(0, s, t, y, tileptr);
    tileptr += spanMap;
    while (y < 100 - 16)
    {
        tileRow(y, s, 0, 16, tileptr);
        tileptr += spanMap;
        y       += 16;
    }
    tileRow(y, s, 0, 100 - y, tileptr);
}
/*
 * Tile into memory buffer
 */
void tileBufRow(int y, unsigned int s, unsigned int t, int height, unsigned char far * far *tileptr, int widthBuf, unsigned char far *buf)
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
void tileBuf(unsigned int s, unsigned int t, int widthBuf, int heightBuf, unsigned char far *buf)
{
    int y;
    unsigned char far * far *tileptr;

    tileptr = tileMap + (t >> 4) * spanMap + (s >> 4);
    s &= 0x0F;
    t &= 0x0F;
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
            y       += 16;
        }
        tileBufRow(y, s, 0, heightBuf - y, tileptr, widthBuf, buf);
    }
}
/*
 * Copy buffer to screen
 */
void cpyBuf(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf)
{
    int w, span;
    unsigned int extS, extT;
    unsigned int pixaddr;

    /*
     * Calc screen extents
     */
    extS = orgS + 158;
    extT = orgT + 99;
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
    CPYBUF((scanline[t - orgT] + (s - orgS) + orgAddr) & 0x3FFF, width >> 1, height, span, buf);
}
unsigned long tileScroll(int scrolldir)
{
    unsigned int hcount, haddr, vaddr;

    if (scrolldir & SCROLL_LEFT2)
    {
        if (orgS < maxOrgS)
        {
            orgS   += 2;
            orgAddr = (orgAddr + 2) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_LEFT2 | SCROLL_RIGHT2);
    }
    else if (scrolldir & SCROLL_RIGHT2)
    {
        if (orgS > 0)
        {
            orgS    = (orgS - 2) & 0x0FFFE;
            orgAddr = (orgAddr - 2) & 0x3FFF;
        }
        else
        scrolldir &= ~(SCROLL_LEFT2 | SCROLL_RIGHT2);
    }
    if (scrolldir & SCROLL_UP2)
    {
        if (orgT < maxOrgT)
        {
            orgT    = (orgT + 2) & 0x0FFFE;
            orgAddr = (orgAddr + 320) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    else if (scrolldir & SCROLL_DOWN2)
    {
        if (orgT > 0)
        {
            orgT    = (orgT - 2) & 0x0FFFE;
            orgAddr = (orgAddr - 320) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    else if (scrolldir & SCROLL_UP)
    {
        if (orgT < maxOrgT + 1)
        {
            orgT++;
            orgAddr = (orgAddr + 160) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    else if (scrolldir & SCROLL_DOWN)
    {
        if (orgT > 0)
        {
            orgT--;
            orgAddr = (orgAddr - 160) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    /*
     * Pre-render edges based on scroll direction
     */
     if (scrolldir & SCROLL_RIGHT2)
     {
         /*
          * Fill in left edge
          */
         tileEdgeV(orgS & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * spanMap + (orgS >> 4));
         vaddr = orgAddr;
     }
     else if (scrolldir & SCROLL_LEFT2)
     {
         /*
          * Fill right edge
          */
         tileEdgeV((orgS + 158) & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * spanMap + ((orgS + 158) >> 4));
         vaddr = (orgAddr + 158) & 0x3FFF;
     }
    if (scrolldir & SCROLL_DOWN2)
    {
        /*
         * Fill in top edge
         */
        tileEdgeH2(orgS & 0x0E, orgT & 0x0E, tileMap + (orgT >> 4) * spanMap + (orgS >> 4));
        hcount = 2;
        haddr  = orgAddr;
    }
    else if (scrolldir & SCROLL_UP2)
    {
        /*
         * Fill in botom edge
         */
        tileEdgeH2(orgS & 0x0E, (orgT + 98) & 0x0E, tileMap + ((orgT + 98) >> 4) * spanMap + (orgS >> 4));
        hcount = 2;
        haddr  = (orgAddr + 98 * 160) & 0x3FFF;
    }
    else if (scrolldir & SCROLL_DOWN)
    {
        /*
         * Fill in top edge
         */
        tileEdgeH(orgS & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * spanMap + (orgS >> 4));
        haddr  = orgAddr;
        hcount = 1;
    }
    else if (scrolldir & SCROLL_UP)
    {
        /*
         * Fill in botom edge
         */
        tileEdgeH(orgS & 0x0E, (orgT + 99) & 0x0F, tileMap + ((orgT + 99) >> 4) * spanMap + (orgS >> 4));
        haddr  = (orgAddr + 99 * 160) & 0x3FFF;
        hcount = 1;
    }
    /*
     * The following happens during VBlank
     */
    setStartAddr(orgAddr >> 1);
    outp(0x3D9, 0x00);
    /*
     * Fill in edges
     */
    if (scrolldir & (SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN))
        _cpyEdgeH(haddr, hcount);
    if (scrolldir & (SCROLL_LEFT2 | SCROLL_RIGHT2))
        _cpyEdgeV(vaddr);
    /*
     * Return updated origin as 32 bit value
     */
    outp(0x3D9, 0x06);
    return ((unsigned long)orgT << 16) | orgS;
}
void tileInit(unsigned int s, unsigned int t, unsigned int width, unsigned int height, unsigned char far * far *map)
{
    tileMap = map;
    spanMap = width;
    maxS    = (width  << 4) - 2;
    maxT    = (height << 4) - 2;
    maxOrgS = maxS - 160;
    maxOrgT = maxT - 100;
    orgS    = s & 0xFFFE; // S always even
    orgT    = t;
    orgAddr = (orgT * 160 + orgS | 1) & 0x3FFF;
    outp(0x3D8, 0x00);  /* Turn off video */
    tileScrn(orgS, orgT);
    outp(0x3D8, 0x09);  /* Turn on video */
    enableRasterTimer(199);
    setStartAddr(orgAddr >> 1);
}
void tileExit(void)
{
    disableRasterTimer();
}
