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
static unsigned char cga160crtc[] = {113, 80, 89, 15, 127, 6, 100, 112, 2, 1, 32, 0, 0, 0};
static unsigned char ega160misc   =  0x23;
static unsigned char ega160seq[]  = {0x03, 0x01, 0x03, 0x00, 0x03};
static unsigned char ega160crtc[] = {0x70, 0x4F, 0x5C, 0x2F, 0x5F, 0x07, 0x04, 0x11, 0x00, 0x01, 0x06, 0x07,
                    /* Start Addr */ 0x00, 0x00, 0x00, 0x00, 0xE1, 0x24, 0xC7, 0x28, 0x08, 0xE0, 0xF0, 0xA3, 0xFF};
/*
 * 8x4 dither matrix (4x4 replicated twice horizontally to fill byte).
 */
#define BRI_PLANE             3
#define RED_PLANE             2
#define GRN_PLANE             1
#define BLU_PLANE             0
static unsigned int ddithmask[16] = // Dark color dither
{
    0x0000,
    0x8000,
    0x8020,
    0x80A0,
    0xA0A0,
    0xA4A0,
    0xA4A1,
    0xA4A5,
    0xA5A5,
    0xADA5,
    0xADA7,
    0xADAF,
    0xAFAF,
    0xEFAF,
    0xEFBF,
    0xEFFF
};
static unsigned int bdithmask[16] = // Bright color dither
{
    0x0000,
    0x8000,
    0x8020,
    0x80A0,
    0xA0A0,
    0xA4A0,
    0xA4A1,
    0xA4A5,
    0xA5A5,
    0xADA5,
    0xADA7,
    0xADAF,
    0xAFAF,
    0xEFAF,
    0xEFBF,
    0xFFFF
};
static unsigned int adapter;
unsigned int orgAddr = 1;
unsigned int scrnMask;
unsigned int scanline[100]; // Precalculated scanline offsets
unsigned char borderColor;

void txt80(void)
{
    union REGS regs;

    regs.x.ax = 3;
    int86(0x10, &regs, &regs);
}
/*
 * Mode set for 160x100 16 color mode
 */
unsigned int gr160(unsigned char fill, unsigned char border)
{
    int i, wfill, chrows;
    int far *wvidmem = (int far *)0xB8000000L;
    union REGS regs;

    /* Start off in text mode 3 */
    regs.x.ax = 3;
    int86(0x10, &regs, &regs);
    regs.x.ax = 0x1130; /* Get character generator information */
    regs.x.bx = 0x0000;
    regs.x.cx = 0x0000;
    int86(0x10, &regs, &regs);
    chrows = regs.h.cl;
    rasterDisable();
    if (chrows)
    {
        scrnMask = 0x7FFF;
        if (chrows != 16)
        {   /* EGA */
            adapter = EGA;
            outp(0x3C4, 0); outp(0x3C5, 0x00); /* Sequencer reset */
            outp(0x3C2, ega160misc); /* reprogram into 200 scanline mode */
            for (i = 1; i < sizeof(ega160seq); i++)
            {
                outp(0x3C4, i); outp(0x3C5, ega160seq[i]);
            }
            for (i = 0; i < sizeof(ega160crtc); i++)
            {
                outp(0x3D4, i); outp(0x3D5, ega160crtc[i]);
            }
    		inp(0x3DA); /* Read the input status register to reset the VGA attribute controller index/data state */
    		outp(0x3c0, 0x10); /* ttribute controller index register, mode register select */
    		i  = inp(0x3C1); /* VGA attribute controller data register, read mode register */
    		i &= 0xF7; /* turn off bit 3, blink */
    		outp(0x3C0, 0x10); /* select the attribute control register */
    		outp(0x3C0, i);	/* write to VGA attribute controller data register */
            outp(0x3C4, 0); outp(0x3C5, 0x03); /* Sequencer reset */
        }
        else
        {   /* VGA */
            adapter = VGA;
            regs.x.ax = 0x1110; /* Load user character set */
            regs.x.bx = (chrows / 4) << 8;
            regs.x.cx = 1;
            regs.x.dx = 0;
            int86(0x10, &regs, &regs); /* Set quarter height characters */
            regs.x.ax = 0x1003;
            regs.x.bx = 0x0000;
            int86(0x10, &regs, &regs); /* turn off blink via BIOS */
        }
    }
    else
    {   /* CGA */
        adapter = CGA;
        scrnMask = 0x3FFF;
        /* Set CRTC registers */
        for (i = 0; i < sizeof(cga160crtc); i++)
        {
            outp(0x3D4, i); outp(0x3D5, cga160crtc[i]);
        }
        borderColor = border & 0x0F;
        rasterBorder(borderColor);
    }
    fill = (fill << 4) | fill;
    wfill = 221 | (fill << 8);
    for (i = 0; i < scrnMask>>1; i++)
        wvidmem[i] = wfill;
    rasterEnable();
    /* Precaclulate scanline offsets */
    for (i = 0; i < 100; i++)
        scanline[i] = i * 160;
    return adapter;
}
void _rect(unsigned int x, unsigned int y, int width, int height, unsigned char color)
{
    unsigned int x2;

    x2 = x + width - 1;
    do
    {
        _hlin(x, x2, y++, color);
    } while (--height);
}
void _rectSnow(unsigned int x, unsigned int y, int width, int height, unsigned char color)
{
    unsigned int x2;

    x2 = x + width - 1;
    do
    {
        _hlinSnow(x, x2, y++, color);
    } while (--height);
}
void _text(unsigned int x, unsigned int y, unsigned char color, char *string)
{
    unsigned int ix, iy;
    unsigned char glyphbits, scanbit;
    unsigned char far *glyphptr;

    while (*string)
    {
        glyphptr = (unsigned char far *)0xFFA6000EL + (unsigned char)*string++ * 8;
        for (iy = y; iy < (y + 8); iy++)
        {
            glyphbits = *glyphptr++;
            if (glyphbits)
            {
                scanbit = 0x80;
                for (ix = x; scanbit; ix++)
                {
                    if (scanbit & glyphbits)
                        _plot(ix, iy, color);
                    scanbit >>= 1;
                }
            }
        }
        x += 8;
    }
}
void _textSnow(unsigned int x, unsigned int y, unsigned char color, char *string)
{
    unsigned int ix, iy;
    unsigned char glyphbits, scanbit;
    unsigned char far *glyphptr;

    while (*string)
    {
        glyphptr = (unsigned char far *)0xFFA6000EL + (unsigned char)*string++ * 8;
        for (iy = y; iy < (y + 8); iy++)
        {
            glyphbits = *glyphptr++;
            if (glyphbits)
            {
                scanbit = 0x80;
                for (ix = x; scanbit; ix++)
                {
                    if (scanbit & glyphbits)
                        _plotSnow(ix, iy, color);
                    scanbit >>= 1;
                }
            }
        }
        x += 8;
    }
}
/*
 * Build a dithered brush and return the closest solid color match to the RGB.
 */
unsigned char brush(unsigned char red, unsigned char grn, unsigned blu, unsigned int *pattern)
{
    unsigned char clr, l, i;

    /*
     * Find MAX(R,G,B)
     */
    if (red >= grn && red >= blu)
        l = red;
    else if (grn >= red && grn >= blu)
        l = grn;
    else // if (blue >= grn && blu >= red)
        l = blu;
    if (l > 127) // 50%-100% brightness
    {
        /*
         * Fill brush based on scaled RGB values (brightest -> 100% -> 0x0F).
         */
        pattern[BRI_PLANE] = bdithmask[(l >> 3) & 0x0F];
        pattern[RED_PLANE] = bdithmask[(red << 4) / (l + 8)];
        pattern[GRN_PLANE] = bdithmask[(grn << 4) / (l + 8)];
        pattern[BLU_PLANE] = bdithmask[(blu << 4) / (l + 8)];
        clr  = 0x08
             | ((red & 0x80) >> 5)
             | ((grn & 0x80) >> 6)
             | ((blu & 0x80) >> 7);
    }
    else // 0%-50% brightness
    {
        /*
         * Fill brush based on dim RGB values.
         */
        if ((l - red) + (l - grn) + (l - blu) < 8)
        {
            /*
             * RGB close to grey.
             */
            if (l > 63) // 25%-50% grey
            {
                /*
                 * Mix light grey and dark grey.
                 */
                pattern[BRI_PLANE] = ~ddithmask[((l - 64) >> 2)];
                pattern[RED_PLANE] =
                pattern[GRN_PLANE] =
                pattern[BLU_PLANE] =  ddithmask[((l - 64) >> 2)];
                clr =  0x07;
            }
            else // 0%-25% grey
            {
                /*
                 * Simple dark grey dither.
                 */
                pattern[BRI_PLANE] = ddithmask[(l >> 2)];
                pattern[RED_PLANE] = 0;
                pattern[GRN_PLANE] = 0;
                pattern[BLU_PLANE] = 0;
                clr = (l > 31) ? 0x08 : 0x00;
            }
        }
        else
        {
            /*
             * Simple 8 color RGB dither.
             */
            pattern[BRI_PLANE] = 0;
            pattern[RED_PLANE] = ddithmask[red >> 3];
            pattern[GRN_PLANE] = ddithmask[grn >> 3];
            pattern[BLU_PLANE] = ddithmask[blu >> 3];
            clr = ((red & 0x40) >> 4)
                | ((grn & 0x40) >> 5)
                | ((blu & 0x40) >> 6);
        }
    }
    return clr;
}
/*
 * This is a horrible way to set a pixel. It builds a 4x4 brush then extracts
 * the 4 bit pixel value from the 4 color planes. Treat the brush as 4 rows of
 * individual IRGB bytes instead of the 4 rows of a combined IRGB long in the
 * build brush routine.
 */
void _plotrgb(int x, int y, unsigned char red, unsigned char grn, unsigned char blu)
{
    unsigned int pixbrush[4];
    unsigned char ofst, pix;

    brush(red, grn, blu, pixbrush);
    /*
     * Extract pixel value from IRGB planes.
     */
    ofst = (4 * (y & 3)) + (x & 3);
    pix  = ((pixbrush[BRI_PLANE] >> ofst) & 0x01) << BRI_PLANE;
    pix |= ((pixbrush[RED_PLANE] >> ofst) & 0x01) << RED_PLANE;
    pix |= ((pixbrush[GRN_PLANE] >> ofst) & 0x01) << GRN_PLANE;
    pix |= ((pixbrush[BLU_PLANE] >> ofst) & 0x01) << BLU_PLANE;
    _plot(x, y, pix);
}
void _plotrgbSnow(int x, int y, unsigned char red, unsigned char grn, unsigned char blu)
{
    unsigned int pixbrush[4];
    unsigned char ofst, pix;

    brush(red, grn, blu, pixbrush);
    /*
     * Extract pixel value from IRGB planes.
     */
    ofst = (4 * (y & 3)) + (x & 3);
    pix  = ((pixbrush[BRI_PLANE] >> ofst) & 0x01) << BRI_PLANE;
    pix |= ((pixbrush[RED_PLANE] >> ofst) & 0x01) << RED_PLANE;
    pix |= ((pixbrush[GRN_PLANE] >> ofst) & 0x01) << GRN_PLANE;
    pix |= ((pixbrush[BLU_PLANE] >> ofst) & 0x01) << BLU_PLANE;
    _plotSnow(x, y, pix);
}


