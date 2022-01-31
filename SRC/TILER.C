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
void cpyEdgeH(int addr, int count);
void cpyEdgeV(int addr);
#ifdef CGA_SNOW
#define TILE    _tile
#else
#define TILE    _tileSnow
#endif
#define tile(x,y,s,t,w,h,p) TILE((scanline[y]+(x)+orgAddr)&0x3FFF,(w)>>1,h,(p)+(t)*8+((s)>>1))
/*
 * Fast memory routines
 */
void tileMem(unsigned int s, unsigned int t, int width, int height, unsigned char far *tile, int span, unsigned char far *buf);
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
unsigned int maxS, maxT, maxOrgS, maxOrgT, extS, extT;
unsigned int widthMap, spanMap;
unsigned char far * far *tileMap;
/*
 * On-the-fly tile updates
 */
int                tileUpdateCount = 0;
unsigned int       tileUpdateS[16];
unsigned int       tileUpdateT[16];
unsigned char far *tileUpdatePtr[16];

void tileRow(int y, unsigned int s, unsigned int t, int height, unsigned char far * far *tileptr)
{
    int x;

    x = 16 - s;
    tile(0, y, s, t, x, height, *tileptr++);
    do {
        tile(x, y, 0, t, 16, height, *tileptr++);
        x += 16;
    } while (x < 160 - 16);
    tile(x, y, 0, t, 160 - x, height, *tileptr);
}
void tileScrn(unsigned int s, unsigned int t)
{
    int y;
    unsigned char far * far *tileptr;

    tileptr = tileMap + (t >> 4) * widthMap + (s >> 4);
    s &= 0x0F;
    t &= 0x0F;
    y  = 16 - t;
    tileRow(0, s, t, y, tileptr);
    tileptr += widthMap;
    do
    {
        tileRow(y, s, 0, 16, tileptr);
        tileptr += widthMap;
        y       += 16;
    } while (y < 100 - 16);
    tileRow(y, s, 0, 100 - y, tileptr);
}
/*
 * Update tile in map
 */
void tileUpdate(unsigned i, unsigned j, unsigned char far *tileNew)
{
    *(tileMap + (j * widthMap) + i) = tileNew;
    //if (tileUpdateCount < 16)
    {
        tileUpdatePtr[tileUpdateCount] = tileNew;
        tileUpdateS[tileUpdateCount]   = i << 4;
        tileUpdateT[tileUpdateCount]   = j << 4;
        tileUpdateCount++;
    }
}
unsigned long tileScroll(int scrolldir)
{
    unsigned int hcount, haddr, vaddr;

    if (scrolldir & SCROLL_LEFT2)
    {
        if (orgS < maxOrgS)
        {
            orgS   += 2;
            extS    = orgS + 160;
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
            extS    = orgS + 160;
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
            extT    = orgT + 100;
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
            extT    = orgT + 100;
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
            extT    = orgT + 100;
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
            extT    = orgT + 100;
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
        tileEdgeV(orgS & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * widthMap + (orgS >> 4));
        vaddr = orgAddr;
    }
    else if (scrolldir & SCROLL_LEFT2)
    {
        /*
         * Fill right edge
         */
        tileEdgeV((orgS + 158) & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * widthMap + ((orgS + 158) >> 4));
        vaddr = (orgAddr + 158) & 0x3FFF;
    }
    if (scrolldir & SCROLL_DOWN2)
    {
        /*
         * Fill in top edge
         */
        tileEdgeH2(orgS & 0x0E, orgT & 0x0E, tileMap + (orgT >> 4) * widthMap + (orgS >> 4));
        hcount = 2;
        haddr  = orgAddr;
    }
    else if (scrolldir & SCROLL_UP2)
    {
        /*
         * Fill in botom edge
         */
        tileEdgeH2(orgS & 0x0E, (orgT + 98) & 0x0E, tileMap + ((orgT + 98) >> 4) * widthMap + (orgS >> 4));
        hcount = 2;
        haddr  = (orgAddr + 98 * 160) & 0x3FFF;
    }
    else if (scrolldir & SCROLL_DOWN)
    {
        /*
         * Fill in top edge
         */
        tileEdgeH(orgS & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * widthMap + (orgS >> 4));
        haddr  = orgAddr;
        hcount = 1;
    }
    else if (scrolldir & SCROLL_UP)
    {
        /*
         * Fill in botom edge
         */
        tileEdgeH(orgS & 0x0E, (orgT + 99) & 0x0F, tileMap + ((orgT + 99) >> 4) * widthMap + (orgS >> 4));
        haddr  = (orgAddr + 99 * 160) & 0x3FFF;
        hcount = 1;
    }
    /*
     * The following happens after last active scanline
     */
    setStartAddr(orgAddr >> 1);
    outp(0x3D9, 0x00);
    /*
     * Fill in edges
     */
    if (scrolldir & (SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN))
        cpyEdgeH(haddr, hcount);
    if (scrolldir & (SCROLL_LEFT2 | SCROLL_RIGHT2))
        cpyEdgeV(vaddr);
    /*
     * Draw any updated tiles
     */
    while (tileUpdateCount)
    {
        unsigned int s, t, width, height;
        tileUpdateCount--;
        /*
         * Quick reject
         */
        if ((tileUpdateS[tileUpdateCount] >= extS) || (tileUpdateS[tileUpdateCount] + 16 <= orgS)
         || (tileUpdateT[tileUpdateCount] >= extT) || (tileUpdateT[tileUpdateCount] + 16 <= orgT))
            continue;
        /*
         * Clip to screen edges
         */
        s      = tileUpdateS[tileUpdateCount];
        t      = tileUpdateT[tileUpdateCount];
        width  =
        height = 16;
        if (s < orgS)
        {
            width = 16 - (orgS - s);
            s     = orgS;
        }
        else if (s + 16 > extS)
            width = extS - s;
        if (t < orgT)
        {
            height = 16 - (orgT - t);
            t      = orgT;
        }
        else if (t + 16 > extT)
            height = extT - t;
        /*
         * BLT to video memory
         */
        tile(s - orgS, t - orgT, s & 0x0F, t & 0x0F, width, height, tileUpdatePtr[tileUpdateCount]);
    }
    /*
     * Return updated origin as 32 bit value
     */
    outp(0x3D9, 0x06);
    return ((unsigned long)orgT << 16) | orgS;
}
void tileInit(unsigned int s, unsigned int t, unsigned int width, unsigned int height, unsigned char far * far *map)
{
    tileMap  = map;
    widthMap = width;
    spanMap  = widthMap << 2;
    maxS     = (width  << 4) - 2;
    maxT     = (height << 4) - 2;
    maxOrgS  = maxS - 160;
    maxOrgT  = maxT - 100;
    orgS     = s & 0xFFFE; // S always even
    orgT     = t;
    extS     = orgS + 160;
    extT     = orgT + 100;
    orgAddr  = (orgT * 160 + orgS | 1) & 0x3FFF;
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
