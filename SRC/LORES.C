#include <dos.h>
#include <conio.h>
#include <math.h>

//char gr160regs[] = {113, 80, 88, 15, 127, 6, 100, 112, 2, 1, 32, 0, 0, 0};
char gr160regs[] = {113, 80, 89, 15, 127, 6, 100, 112, 2, 1, 32, 0, 0, 0};
unsigned int scanline[100]; // Precalculated scanline offsets
unsigned char borderColor;

void txt80(void)
{
    union REGS regs;

    regs.x.ax = 2;
    int86(0x10, &regs, &regs);
}
void gr160(unsigned char fill, unsigned char border)
{
    int i, wfill;
    int far *wvidmem = (int far *)0xB8000000L;
    /* Set CRTC registers */
    outp(0x3D8, 0x00); /* Turn off video */
    for (i = 0; i < 14; i++)
    {
        outp(0x3D4, i); outp(0x3D5, gr160regs[i]);
    }
    outp(0x3D8, 0x09);      // Turn off blink attribute
    outp(0x3D9, border);    // Set border
    fill = (fill << 4) | fill;
    wfill = 221 | (fill << 8);
    for (i = 0; i < 8192; i++)
        wvidmem[i] = wfill;
    /*
     * Precaclulate scanline offsets
     */
    for (i = 0; i < 100; i++)
        scanline[i] = i * 160;
}
void plot(unsigned int x, unsigned int y, unsigned char color)
{
    unsigned char far *vidmem = (unsigned char far *)0xB8000000L + (scanline[y] + x | 1);
    if (x & 1)
        *vidmem = (*vidmem & 0x0F) | (color << 4);
    else
        *vidmem = (*vidmem & 0xF0)  | color;
}
int rect(unsigned int x, unsigned int y, int width, int height, unsigned char color)
{
    char lcolor, dcolor;
    unsigned char far *vidmem = (unsigned char far *)0xB8000000L + (scanline[y] + x | 1);
    int w;
    lcolor = color << 4;
    dcolor = lcolor | color;
    if (x & 1)
    {
        while (height--)
        {
            vidmem[0] = (vidmem[0] & 0x0F) | lcolor;
            if (!(width & 1))
                vidmem[width] = (vidmem[width] & 0xF0) | color;
            for (w = (width - 1) & 0xFFFE; w >= 2; w -= 2)
                vidmem[w] = dcolor;
            vidmem += 160;
        }
    }
    else
    {
        while (height--)
        {
            if (width & 1)
                vidmem[width - 1] = (vidmem[width - 1] & 0xF0) | color;
            for (w = (width - 2) & 0xFFFE; w >= 0; w -= 2)
                vidmem[w] = dcolor;
            vidmem += 160;
        }
    }
}
void hlin(unsigned int x1, unsigned int x2, unsigned int y, unsigned char color)
{
    unsigned int x;
    unsigned char hcolor, dcolor;
    unsigned char far *vidmem = (unsigned char far *)0xB8000000L + (scanline[y] | 1);
    hcolor = color << 4;
    dcolor = hcolor | color;
    if (x1 & 1)
    {
        vidmem[x1 & 0xFE] = (vidmem[x1 & 0xFE] & 0x0F) | hcolor;
        x1++;
    }
    if (!(x2 & 1))
    {
        vidmem[x2] = (vidmem[x2] & 0xF0) | color;
        x2--;
    }
    for (x = x1; x < x2; x += 2)
        vidmem[x] = dcolor;
}
void vlin(unsigned int x, unsigned int y1, unsigned int y2, unsigned char color)
{
    unsigned char far *vidmem = (unsigned char far *)0xB8000000L + (scanline[y1] + x | 1);
    if (x & 1)
    {
        color <<= 4;
        while (y1++ <= y2)
        {
            *vidmem = (*vidmem & 0x0F) | color;
            vidmem += 160;
        }
    }
    else
        while (y1++ <= y2)
        {
            *vidmem = (*vidmem & 0xF0)  | color;
            vidmem += 160;
        }
}
//#define FAST_MULDIV
void line(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned char color)
{
    int dx2, dy2, err, sx, sy;
    int shorterr, shortlen, longerr, longlen, halflen;
    unsigned int ps;
#ifdef FAST_MULDIV
    div_t result;
#endif

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
#ifdef FAST_MULDIV
        result  =  div(dx2 / 2, dy2); // Find first half-span length and error
        halflen = result.quot;
        err     = dy2 - result.rem;
        x1     += halflen;
#else
        err = dy2 - dx2 / 2;
        while (err < 0) // Find first half-span length and error
        {
            err += dy2;
            x1++;
        }
#endif
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
#ifdef FAST_MULDIV
        result  = div(dy2 / 2, dx2);
        halflen = result.quot;
        err     = dx2 - result.rem;
        y1     += halflen;
#else
        err = dx2 - dy2 / 2;
        while (err < 0)
        {
            err += dx2;
            y1++;
        }
#endif
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


