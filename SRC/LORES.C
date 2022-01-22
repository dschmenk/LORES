#include <dos.h>
#include <conio.h>

char gr160regs[] = {113, 80, 88, 15, 127, 6, 100, 112, 2, 1, 32, 0, 0, 0};

void txt80(void)
{
    union REGS regs;

    regs.x.ax = 2;
    int86(0x10, &regs, &regs);
}
void gr160(unsigned char color)
{
    int i, fill;
    int far *vidmemw = (int far *)0xB8000000L;
    /* Set CRTC registers */
    outp(0x3D8, 0x00); /* Turn off video */
    for (i = 0; i < 14; i++)
    {
        outp(0x3D4, i); outp(0x3D5, gr160regs[i]);
    }
    outp(0x3D8, 0x09);      /* Turn off blink attribute */
    outp(0x3D9, 0x06);      /* Yellow border to get colorburst working */
    color = (color << 4) | color;
    fill = 221 | (color << 8);
    for (i = 0; i < 8192; i++)
        vidmemw[i] = fill;
}
void plot(unsigned int x, unsigned int y, unsigned char color)
{
    unsigned char far *vidmem = (unsigned char far *)(0xB8000000L | (y * 160) + (x | 1));
    if (x & 1)
        *vidmem = (*vidmem & 0x0F) | (color << 4);
    else
        *vidmem = (*vidmem & 0xF0)  | color;
}
int rect(unsigned int x, unsigned int y, int width, int height, unsigned char color)
{
    char lcolor, dcolor;
    unsigned char far *vidmem = (unsigned char far *)(0xB8000000L | (y * 160) + (x | 1));
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

