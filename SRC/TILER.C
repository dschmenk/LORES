#include <stdio.h>
#include <dos.h>
#include <conio.h>
/*
 * Fast edge fill routines
 */
void setstartaddr(int addr);
void filledgeh(int addr, unsigned char *edge);
void filledgev(int addr, unsigned char *edge);

/*
 * Map and tile data
 */
unsigned char testile[] = {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0x9F, 0x99, 0x99, 0x99, 0x99, 0xF9, 0x22,
                           0x22, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x22,
                           0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22};
unsigned char grass[] = {  0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x2A, 0x22, 0x22, 0x22, 0x22, 0x32, 0x22,
                           0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x22, 0x22, 0x32, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x22, 0x22, 0x22, 0xA2, 0x22, 0x22, 0xA2,
                           0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x22, 0x32, 0x22, 0x22, 0x22, 0x2A, 0x22,
                           0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x22, 0x22, 0x2A, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x2A, 0x22, 0x22, 0x23, 0x22, 0x22, 0x22,
                           0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22};
unsigned char tree[] = {   0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
                           0x22, 0x22, 0xA2, 0xAA, 0x2A, 0x2A, 0x22, 0x22,
                           0x22, 0xA2, 0xAA, 0xAA, 0xAA, 0xAA, 0x2A, 0x22,
                           0x22, 0xA2, 0xAA, 0xAA, 0xAA, 0xAA, 0x2A, 0x22,
                           0x22, 0x22, 0x2A, 0xA2, 0x2A, 0xA2, 0x2A, 0x22,
                           0x22, 0xAA, 0x2A, 0xAA, 0xAA, 0xA2, 0xAA, 0x22,
                           0x22, 0xAA, 0xAA, 0xAA, 0xAA, 0x2A, 0xAA, 0x22,
                           0x22, 0xA2, 0xAA, 0xAA, 0xA2, 0x2A, 0xAA, 0x22,
                           0x22, 0xAA, 0xAA, 0xAA, 0xA2, 0x2A, 0x2A, 0x22,
                           0x22, 0xA2, 0x2A, 0x22, 0xAA, 0xA2, 0xAA, 0x22,
                           0x22, 0xAA, 0xAA, 0xAA, 0xAA, 0x2A, 0xAA, 0x22,
                           0x22, 0x22, 0xAA, 0xAA, 0xAA, 0x22, 0xAA, 0x22,
                           0x22, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x2A, 0x22,
                           0x22, 0xA2, 0xA2, 0xAA, 0xAA, 0xAA, 0x2A, 0x22,
                           0x22, 0x22, 0xA2, 0xAA, 0x22, 0xAA, 0x22, 0x22,
                           0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22};
unsigned char dirt[] = {   0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x76,
                           0x66, 0x60, 0x66, 0x66, 0x66, 0x60, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x67, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x06, 0x66, 0x76, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x06, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x06, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                           0x66, 0x66, 0x66, 0x66, 0x66, 0x76, 0x66, 0x66};
unsigned char water[] = {  0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                           0x11, 0x99, 0x11, 0x99, 0x11, 0x11, 0x11, 0x11,
                           0x11, 0x11, 0x99, 0x11, 0x99, 0x11, 0x11, 0x11,
                           0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                           0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                           0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                           0x11, 0x11, 0x11, 0x11, 0x11, 0x99, 0x11, 0x99,
                           0x99, 0x11, 0x11, 0x11, 0x11, 0x11, 0x99, 0x11,
                           0x11, 0x11, 0x77, 0x11, 0x77, 0x11, 0x11, 0x11,
                           0x11, 0x11, 0x11, 0x77, 0x11, 0x77, 0x11, 0x11,
                           0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                           0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                           0x11, 0x11, 0x11, 0x99, 0x11, 0x99, 0x11, 0x11,
                           0x11, 0x11, 0x11, 0x11, 0x99, 0x11, 0x99, 0x11,
                           0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                           0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
unsigned char *tilemap[16][16];
#define    MAPSPAN    16
/*
 * Build map from text representation
 *  . = grass
 *  * = tree
 *  # = dirt
 *  ~ = water
 */
unsigned char *map[] = {"................",
                        "..***...###.....",
                        ".****..##.###...",
                        "***~**.#....###.",
                        "**~~~*.#...~~~#~",
                        "~~~?~~~#~~~~..#.",
                        "*~~~**.#....###.",
                        ".****..#..###...",
                        ".****..####.....",
                        "..**..##........",
                        ".*...##..****...",
                        ".#####..*....*..",
                        "##.*...*.#..#.*.",
                        "#......*......*.",
                        "##.*....*.##.*..",
                        ".###.....****..."};
void buildmap(void)
{
    int row, col;
    unsigned char *tileptr;

    for (row = 0; row < 16; row++)
    {
        for (col = 0; col < 16; col++)
        {
            switch (map[row][col])
            {
                case '.':
                    tileptr = grass;
                    break;
                case '*':
                    tileptr = tree;
                    break;
                case '#':
                    tileptr = dirt;
                    break;
                case '~':
                    tileptr = water;
                    break;
                default:
                    tileptr = testile;
            }
            tilemap[row][col] = tileptr;
        }
    }
}
/*
 * Graphics routines for 160x100x16 color mode
 */
unsigned char far *vidmem = (unsigned char far *)0xB8000000L;
unsigned char edgeh[2][80], edgev[100];
char gr160regs[] = {113, 80, 88, 15, 127, 6, 100, 112, 2, 1, 32, 0, 0, 0};
int xorg = 0;
int yorg = 0;

int txt80(void)
{
    union REGS regs;

    regs.x.ax = 2;
    int86(0x10, &regs, &regs);
}
int gr160(unsigned char color)
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
void setorg(int x, int y)
{
    int startaddr;
    int delay;

    xorg = x & 0xFF;
    yorg = y & 0xFF;
    startaddr = (y * 80 + (x >> 1)) & 0x3FFF;
    /*
     * Wait for vertical refresh
     */
        while (inp(0x3DA) & 0x08);
        while (!(inp(0x3DA) & 0x08));
    /*
     * Update CRTC
     */
    outp(0x3D4, 12); outp(0x3D5, startaddr >> 8);
    outp(0x3D4, 13); outp(0x3D5, startaddr);
}
int tile(int x, int y, int s, int t, int width, int height, unsigned char *tile)
{
    int w;
    int pixaddr;

    pixaddr = (y * 160 + x | 1) & 0x3FFF;
    tile   += t * 8 + (s >> 1);
    width >>= 1;
    while (height--)
    {
        for (w = 0; w < width; w++)
            vidmem[pixaddr + (w << 1)] = tile[w];
        pixaddr = (pixaddr + 160) & 0x3FFF;
        tile   += 8;
    }
}
void tilerow(int y, int s, int t, int height, unsigned char **tileptr)
{
    int x;

    tile(xorg, y + yorg, s, t, 16 - s, height, *tileptr++);
    for (x = 16 - s; x < 160 - 16 - 1; x += 16)
        tile(x + xorg, y + yorg, 0, t, 16, height, *tileptr++);
    tile(x + xorg, y + yorg, 0, t, 160 - x, height, *tileptr++);
}
void tilecol(int x, int s, int t, int width, unsigned char **tileptr)
{
    int y;

    tile(x + xorg, yorg, s, t, width, 16 - t, *tileptr);
    tileptr += MAPSPAN;
    for (y = 16 - t; y < 100 - 16 - 1; y += 16)
    {
        tile(x + xorg, y + yorg, s, 0, width, 16, *tileptr);
        tileptr += MAPSPAN;
    }
    tile(x + xorg, y + yorg, s, 0, width, 100 - y, *tileptr);
}
void tilescrn(int s, int t, unsigned char **tileptr)
{
    int y;

    tilerow(0, s, t, 16 - t, tileptr);
    tileptr += MAPSPAN;
    for (y = 16 - t; y < 100 - 16 - 1; y += 16)
    {
        tilerow(y, s, 0, 16, tileptr);
        tileptr += MAPSPAN;
    }
    tilerow(y, s, 0, 100 - y, tileptr);
}
/*
 * Tile edge into memory buffer
 */
int tilememh(int x, int s, int t, int width, unsigned char *tile)
{
    int w;

    tile   += t * 8 + (s >> 1);
    x     >>= 1;
    width >>= 1;
    for (w = 0; w < width; w++)
    {
        edgeh[0][x + w] = tile[w];
        edgeh[1][x + w] = tile[w + 8];
    }
}
int tilememv(int y, int s, int t, int height, unsigned char *tile)
{
    int w;
    int pixaddr;

    tile   += t * 8 + (s >> 1);
    while (height--)
    {
        edgev[y++] = *tile;
        tile      += 8;
    }
}
void tileedgeh(int s, int t, unsigned char **tileptr)
{
    int x;

    tilememh(0, s, t, 16 - s, *tileptr++);
    for (x = 16 - s; x < 160 - 16 - 1; x += 16)
        tilememh(x, 0, t, 16, *tileptr++);
    tilememh(x, 0, t, 160 - x, *tileptr++);
}
void tileedgev(int s, int t, unsigned char **tileptr)
{
    int y;

    tilememv(0, s, t, 16 - t, *tileptr);
    tileptr += MAPSPAN;
    for (y = 16 - t; y < 100 - 16 - 1; y += 16)
    {
        tilememv(y, s, 0, 16, *tileptr);
        tileptr += MAPSPAN;
    }
    tilememv(y, s, 0, 100 - y, *tileptr);
}
/*
 * Demo tiling and scrolling screen
 */
int main(int argc, char **argv)
{
    int i;
    int s, t, sinc, tinc;
    int haddr, vaddr;

    buildmap();
    gr160(0);
    /*
     * Set initial coordinates and scroll direction.
     * Scroll increment must be power of two.
     */
    s = 2; sinc =  2;
    t = 8; tinc = -2;
    /*
     * Draw full screen at start address
     */
    setorg(s, t);
    tilescrn(s & 0x0F, t & 0x0F, &tilemap[t >> 4][s >> 4]);
    /*
     * Use hardware scrolling
     */
    while (!kbhit())
    {
        /*
         * Constrain to maximum screen boundary and update origin
         */
        if (s == (((15 << 4) | 14) - 160) || s == 0) sinc = -sinc;
        if (t == (((15 << 4) | 14) - 100) || t == 0) tinc = -tinc;
        s += sinc;
        t += tinc;
        xorg = s & 0xFF;
        yorg = t & 0xFF;
        /*
         * Pre-render edges based on scroll direction
         */
        if (tinc < 0)
        {
            tileedgeh(s & 0x0F, t & 0x0F, &tilemap[t >> 4][s >> 4]);
            haddr = (yorg * 160 + xorg | 1) & 0x3FFF;
        }
        else if (tinc > 0)
        {
            tileedgeh(s & 0x0F, (t + 98) & 0x0F, &tilemap[(t + 98) >> 4][s >> 4]);
            haddr = ((yorg + 98) * 160 + xorg | 1) & 0x3FFF;
        }
        if (sinc < 0)
        {
            tileedgev(s & 0x0F, t & 0x0F, &tilemap[t >> 4][s >> 4]);
            vaddr = (yorg * 160 + xorg | 1) & 0x3FFF;
        }
        else if (sinc > 0)
        {
            tileedgev((s + 158) & 0x0F, t & 0x0F, &tilemap[t >> 4][(s + 158) >> 4]);
            vaddr = (yorg * 160 + xorg + 158 | 1) & 0x3FFF;
        }
        /*
         * The following happens during VBlank
         */
        setstartaddr(t * 80 + (s >> 1));
        outp(0x3D9, 0x00); /* Black border for timing */
        /*
         * Fill in edges
         */
        if (tinc)
            filledgeh(haddr, edgeh[0]);
        if (sinc)
            filledgev(vaddr, edgev);
        outp(0x3D9, 0x06);      /* Yellow border to get colorburst working */
    }
    getch();
    txt80();
    return 0;
}
