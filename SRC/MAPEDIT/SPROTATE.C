#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <bios.h>
#include <dos.h>
#include <conio.h>
#include "mapio.h"
#define SCREEN_WIDTH    40
#define SCREEN_HEIGHT   24
#define STATUS_LINE     SCREEN_HEIGHT
#define CENTER_X        (SCREEN_WIDTH/2)
#define CENTER_Y        (SCREEN_HEIGHT/2)
#define PIx2            (3.14159265358979323846*2.0)
unsigned char far *vidmem = (char far *)0xB8000000L;
unsigned char far *disptr;
int spriteWidth, spriteHeight, spriteCount, sizeofSprite;
unsigned char far *spritePage;
void gotoxy(int x, int y)
{
    union REGS regs;

    regs.x.ax = 0x0200;
    regs.x.bx = 0x0000;
    regs.x.dx = (y << 8)| x;
    int86(0x10, &regs, &regs);
}
void txt80(void)
{
    union REGS regs;

    regs.x.ax = 0x0003;     /* 40x25 color alphanumerics mode */
    int86(0x10, &regs, &regs);
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
    gotoxy(0, 25); /* move cursor off-screen */
}
void plot40(unsigned int x, unsigned int y, unsigned char color)
{
    unsigned int pixaddr = (y * 80) + (x * 2);
    vidmem[pixaddr]     = (color == 8) ? 0xB1 : ' ';
    vidmem[pixaddr + 1] = color << 4;
}
void sprite40(unsigned char far *spriteptr)
{
    int x, y;
    unsigned char pixlo, pixhi, charlo, charhi;

    for (y = 0; y < spriteHeight; y++)
    {
        for (x = 0; x < spriteWidth; x += 2)
        {
            pixlo = *spriteptr << 4;
            pixhi = *spriteptr++ & 0xF0;
            charlo = pixlo == 0x80 ? 0xB1 : ' ';
            charhi = pixhi == 0x80 ? 0xB1 : ' ';
            disptr[y * 80 + (x << 1) + 0] = charlo;
            disptr[y * 80 + (x << 1) + 1] = pixlo;
            disptr[y * 80 + (x << 1) + 2] = charhi;
            disptr[y * 80 + (x << 1) + 3] = pixhi;
        }
    }
}
/*
 * Rotate sprite image n times
 */
void plot(int x, int y, unsigned char color, unsigned char far *buf, int width, int height)
{
    if (x < 0 || x >= spriteWidth || y < 0 || y >= spriteHeight)
        return;
    buf += (y * width + x)/2;
    if (x & 1)
        *buf = (*buf & 0x0F) | (color << 4);
    else
        *buf = (*buf & 0xF0) | color;
}
unsigned char pixel(int x, int y, unsigned char far *buf, int width, int height)
{
    unsigned char color;

    if (x < 0 || x >= spriteWidth || y < 0 || y >= spriteHeight)
        return 0x08;  // transparent
    buf += (y * width + x)/2;
    if (x & 1)
        color = *buf >> 4;
    else
        color = *buf & 0x0F;
    return color;
}

int main(int argc, char **argv)
{
    int i, x, y, rotCount, rotX, rotY, sizeofSprite;
    unsigned char *spriteRot, *spriteptr;
    double angle, sine, cosine, spriteOrgX, spriteOrgY;

    if (argc > 2)
    {
        spriteCount  = spriteLoad(argv[1], (unsigned char far * *)&spritePage, &spriteWidth, &spriteHeight);
        sizeofSprite = spriteWidth * spriteHeight / 2;
        rotCount  = atoi(argv[2]);
        spriteRot = malloc(sizeofSprite * rotCount);
    }
    else
    {
        fprintf(stderr, "Usage: SPROTATE <spritefile> <rotate count>\n");
        exit(1);
    }
#ifdef INTERACTIVE
    txt40();
#endif
    disptr     = vidmem + (SCREEN_HEIGHT - spriteHeight) / 2 * 80 + (SCREEN_WIDTH - spriteWidth);
    spriteOrgX = (double)spriteWidth  / 2.0 - 0.5;
    spriteOrgY = (double)spriteHeight / 2.0 - 0.5;
    spriteptr  = spriteRot;
    for (i = rotCount; i; i--)
    {
        angle  = PIx2 * (double)i / (double)rotCount;
        sine   = sin(angle);
        cosine = cos(angle);
        for (y = 0; y < spriteHeight; y++)
            for (x = 0; x < spriteWidth; x++)
            {
                rotX = floor(cosine * ((double)x - spriteOrgX) - sine   * ((double)y - spriteOrgY) + spriteOrgX + 0.5);
                rotY = floor(sine   * ((double)x - spriteOrgX) + cosine * ((double)y - spriteOrgY) + spriteOrgY + 0.5);
                plot(x, y, pixel(rotX, rotY, spritePage, spriteWidth, spriteHeight), spriteptr, spriteWidth, spriteHeight);
            }
#ifdef INTERACTIVE
        sprite40(spriteptr);
        getch();
#endif
        spriteptr += sizeofSprite;
    }
    spriteSave(argv[1], spriteRot, spriteWidth, spriteHeight, rotCount);
#ifdef INTERACTIVE
    txt80();
#endif
}
