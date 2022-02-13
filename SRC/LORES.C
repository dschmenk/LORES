#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include "lores.h"
/*
 * CGA/EGA/VGA Registers
 */
#define REG_MISC        0x03C2
#define REG_SEQ         0x03C4
#define REG_GC          0x03CE
#define REG_CRTC        0x03D4
#define REG_STATUS      0x03DA
/*
 * CGA CRTC Registers
 */
char gr160regs[] = {113, 80, 89, 15, 127, 6, 100, 112, 2, 1, 32, 0, 0, 0};
unsigned int scanline[100]; // Precalculated scanline offsets
unsigned char borderColor;
unsigned int scrnMask;

void txt40(void)
{
    union REGS regs;

    regs.x.ax = 1;
    int86(0x10, &regs, &regs);
}
void txt80(void)
{
    union REGS regs;

    regs.x.ax = 3;
    int86(0x10, &regs, &regs);
}
/*
 * Mode set for 160x100 16 color mode
 */
int gr160(unsigned char fill, unsigned char border)
{
    int i, wfill, charrows, adapter;
    int far *wvidmem = (int far *)0xB8000000L;
    union REGS regs;

    /* Start off in text mode 3 */
    regs.x.ax = 3;
    int86(0x10, &regs, &regs);
    /* Get character generator information */
    regs.x.ax = 0x1130;
    regs.x.bx = 0x0000;
    regs.x.cx = 0x0000;
    int86(0x10, &regs, &regs);
    charrows = regs.h.cl;
    if (charrows)
    {
        regs.x.ax = 0x1110; /* Load user character set */
        regs.x.bx = (charrows / 4) << 8;
        regs.x.cx = 1;
        regs.x.dx = 0;
        int86(0x10, &regs, &regs); /* Set quearter height characters */
        regs.x.ax = 0x1003;
        regs.x.bx = 0x0000;
        int86(0x10, &regs, &regs); /* turn off blink via EGA/VGA BIOS */
        scrnMask = 0x7FFF;
        adapter = NOT_CGA;
    }
    else
    { /* CGA */
        /* Set CRTC registers */
        outp(0x3D8, 0x00); /* Turn off video */
        for (i = 0; i < 14; i++)
        {
            outp(0x3D4, i); outp(0x3D5, gr160regs[i]);
        }
        outp(0x3D8, 0x09);      // Turn off blink attribute
        borderColor = border & 0x0F;
        rasterBorder(borderColor);    // Set border
        scrnMask = 0x3FFF;
        adapter = CGA;
    }
    fill = (fill << 4) | fill;
    wfill = 221 | (fill << 8);
    for (i = 0; i < scrnMask; i++)
        wvidmem[i] = wfill;
    /*
     * Precaclulate scanline offsets
     */
    for (i = 0; i < 100; i++)
        scanline[i] = i * 160;
    return adapter;
}
void rect(unsigned int x, unsigned int y, int width, int height, unsigned char color)
{
    unsigned int x2;

    x2 = x + width - 1;
    do
    {
        hlin(x, x2, y++, color);
    } while (--height);
}
void line(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned char color)
{
    int dx2, dy2, err, sx, sy;
    int shorterr, shortlen, longerr, longlen, halflen;
    unsigned int ps;
    div_t result;

    sx = sy = 1;
    if ((dx2 = (x2 - x1) * 2) < 0)
    {
        sx  = -1;
        dx2 = -dx2;
    }
    if ((dy2 = (y2 - y1) * 2) < 0)
    {
        sy  = -1;
        dy2 = -dy2;
    }
    if (dx2 >= dy2)
    {
        if (sx < 0)
        {
            ps = x1; x1 = x2; x2 = ps;
            ps = y1; y1 = y2; y2 = ps;
            sy = -sy;
        }
        if (dy2 == 0)
        {
            hlin(x1, x2, y1, color);
            return;
        }
        ps  = x1;
        result  =  div(dx2 / 2, dy2); // Find first half-span length and error
        halflen = result.quot;
        err     = dy2 - result.rem;
        x1     += halflen;
        longlen = (x1 - ps + 1) * 2; // Long-span length = half-span length * 2
        longerr = err * 2;
        if (longerr >= dy2)
        {
            longerr -= dy2;
            longlen--;
        }
        shortlen = longlen - 1; // Short-span length = long-span length - 1
        shorterr = longerr - dy2;
        err     += shorterr; // Do a short-span step
        while (x1 < x2)
        {
            hlin(ps, x1, y1, color);
            y1 += sy;     // Move to next span
            ps  = x1 + 1; // Start of next span = end of previous span + 1
            if (err >= 0) // Short span
            {
                err += shorterr;
                x1  += shortlen;
            }
            else          // Long span
            {
                err += longerr;
                x1  += longlen;
            }
        }
        hlin(ps, x2, y2, color); // Final span
    }
    else
    {
        if (sy < 0)
        {
            ps = x1; x1 = x2; x2 = ps;
            ps = y1; y1 = y2; y2 = ps;
            sx = -sx;
        }
        if (dx2 == 0)
        {
            vlin(x1, y1, y2, color);
            return;
        }
        ps  = y1;
        result  = div(dy2 / 2, dx2);
        halflen = result.quot;
        err     = dx2 - result.rem;
        y1     += halflen;
        longlen = (y1 - ps + 1) * 2;
        longerr = err * 2;
        if (longerr >= dx2)
        {
            longerr -= dx2;
            longlen--;
        }
        shortlen = longlen - 1;
        shorterr = longerr - dx2;
        err     += shorterr;
        while (y1 < y2)
        {
            vlin(x1, ps, y1, color);
            x1 += sx;
            ps  = y1 + 1;
            if (err >= 0) // Short span
            {
                err += shorterr;
                y1  += shortlen;
            }
            else          // Long span
            {
                err += longerr;
                y1  += longlen;
            }
        }
        vlin(x2, ps, y2, color); // Final span
    }
}
void text(unsigned int x, unsigned int y, unsigned char color, char *string)
{
    unsigned int ix, iy;
    unsigned char glyphbits, scanbit;
    unsigned char far *glyphptr;

    while (*string)
    {
        glyphptr = (unsigned char far *)0xFFA6000EL + (unsigned char)*string++ * 8;
        for (iy = y; iy < (y + 8); iy++)
        {
            if (glyphbits = *glyphptr++)
            {
                scanbit = 0x80;
                for (ix = x; scanbit; ix++)
                {
                    if (scanbit & glyphbits)
                        plot(ix, iy, color);
                    scanbit >>= 1;
                }
            }
        }
        x += 8;
    }
}


