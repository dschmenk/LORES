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
    int far *vidmemw;

    vidmemw = (int far *)0xB8000000L;
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
