#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include "lores.h"
#include "tiler.h"

void simpleline(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned char color)
{
    int dx2, dy2, err, sx, sy;
    unsigned int ps;

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
        err = dy2 - dx2 / 2;
        do
        {
            if (err >= 0)
            {
                hlin(ps, x1, y1, color);
                ps   = x1;
                err -= dx2;
                y1  += sy;
            }
            err += dy2;
            x1++;
        } while (x1 < x2);
        hlin(ps, x2, y2, color);
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
        err = dx2 - dy2 / 2;
        do
        {
            if (err >= 0)
            {
                vlin(x1, ps, y1, color);
                ps   = y1;
                err -= dy2;
                x1  += sx;
            }
            err += dx2;
            y1++;
        } while (y1 < y2);
        vlin(x2, ps, y2, color);
    }
}
void fastline(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned char color)
{
    int dx2, dy2, err, sx, sy;
    int shorterr, shortlen, longerr, longlen, halflen;
    unsigned int ps;
    div_t result;

    sx = sy = 1;
    if ((dx2 = (x2 - x1) << 1) < 0)
    {
        sx  = -1;
        dx2 = -dx2;
    }
    if ((dy2 = (y2 - y1) << 1) < 0)
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
        result  =  div(dx2 >> 1, dy2); // Find first half-span length and error
        halflen = result.quot;
        err     = dy2 - result.rem;
        x1     += halflen;
        longlen = (x1 - ps + 1) << 1; // Long-span length = half-span length * 2
        longerr = err << 1;
        if (longerr >= dy2)
        {
            longerr -= dy2;
            longlen--;
        }
        shortlen = longlen - 1; // Short-span length = long-span length - 1
        shorterr = longerr - dy2;
        err     += shorterr; // Do a short-span step
        do
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
        } while (x1 < x2);
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
        result  = div(dy2 >> 1, dx2);
        halflen = result.quot;
        err     = dx2 - result.rem;
        y1     += halflen;
        longlen = (y1 - ps + 1) << 1;
        longerr = err << 1;
        if (longerr >= dx2)
        {
            longerr -= dx2;
            longlen--;
        }
        shortlen = longlen - 1;
        shorterr = longerr - dx2;
        err     += shorterr;
        do
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
        } while (y1 < y2);
        vlin(x2, ps, y2, color); // Final span
    }
}

int main(int argc, char **argv)
{
    unsigned int fastFrames, simpleFrames, asmFrames;
    int i, x, y;
    gr160(BLACK, BLACK);
    enableRasterTimer(200);
    frameCount = 0;
    for (i = 1; i <= 64; i++)
    {
        for (x = 0; x <= i; x++)
            simpleline(0, 0, x, i, i & 0x0F);
        for (y = 0; y <= i; y++)
            simpleline(0, 0, i, y, i & 0x0F);
    }
    simpleFrames = frameCount;
    frameCount = 0;
    for (i = 1; i <= 64; i++)
    {
        for (x = 0; x <= i; x++)
            fastline(0, 0, x, i, i & 0x0F);
        for (y = 0; y <= i; y++)
            fastline(0, 0, i, y, i & 0x0F);
    }
    fastFrames = frameCount;
    frameCount = 0;
    for (i = 1; i <= 64; i++)
    {
        for (x = 0; x <= i; x++)
            line(0, 0, x, i, i & 0x0F);
        for (y = 0; y <= i; y++)
            line(0, 0, i, y, i & 0x0F);
    }
    asmFrames = frameCount;
    disableRasterTimer();
    txt80();
    printf("simple lines = %u frames\nfast lines = %u frames\nasm lines = %u frames\n", simpleFrames, fastFrames, asmFrames);
}
