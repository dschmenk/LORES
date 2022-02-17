#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#define ESCAPE          0x001B
#define LEFT_ARROW      0x4B00
#define RIGHT_ARROW     0x4D00
#define UP_ARROW        0x4800
#define DOWN_ARROW      0x5000
#define SCREEN_WIDTH    40
#define SCREEN_HEIGHT   24
#define STATUS_LINE     SCREEN_HEIGHT
#define CENTER_X        (SCREEN_WIDTH/2)
#define CENTER_Y        (SCREEN_HEIGHT/2)
#define TILE_WIDTH      16
#define TILE_HEIGHT     16
#define MAP_WIDTH       16
#define MAP_HEIGHT      16
int mapWidth  = MAP_WIDTH;
int mapHeight = MAP_HEIGHT;
unsigned char far *vidmem = (char far *)0xB8000000L;
unsigned char *tileMap[MAP_HEIGHT * MAP_WIDTH];
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
unsigned char *expandTile(unsigned char *tileptr)
{
    int row, col;
    unsigned char *expTile;
    expTile = malloc(16*16);
    for (row = 0; row < TILE_HEIGHT; row++)
    {
        for (col = 0; col < TILE_WIDTH; col += 2)
        {
            expTile[row * 16 + col]     = tileptr[row * 8 + col / 2] & 0x0F;
            expTile[row * 16 + col + 1] = tileptr[row * 8 + col / 2] >> 4;
        }
    }
    return expTile;
}
void buildmap(void)
{
    int row, col;
    unsigned char *tileptr, *grassExp, *treeExp, *dirtExp, *waterExp;

    grassExp = expandTile(grass);
    treeExp  = expandTile(tree);
    dirtExp  = expandTile(dirt);
    waterExp = expandTile(water);
    for (row = 0; row < mapHeight; row++)
    {
        for (col = 0; col < mapWidth; col++)
        {
            switch (map[row][col])
            {
                case '.':
                    tileptr = grassExp;
                    break;
                case '*':
                    tileptr = treeExp;
                    break;
                case '#':
                    tileptr = dirtExp;
                    break;
                case '~':
                    tileptr = waterExp;
                    break;
                default:
                    tileptr = NULL;
                    break;
            }
            tileMap[row * mapWidth + col] = tileptr;
        }
    }
}
unsigned short extgetch(void)
{
    unsigned short extch;

    extch = getch();
    if (!extch)
        extch = getch() << 8;
    return extch;
}
void txt40(void)
{
    union REGS regs;

    regs.x.ax = 1;
    int86(0x10, &regs, &regs);
    outp(0x3D8, 0x09);      // Turn off blink attribute on CGA
    regs.x.ax = 0x1003;
    regs.x.bx = 0x0000;
    int86(0x10, &regs, &regs); /* turn off blink via EGA/VGA BIOS */
    regs.x.ax = 0x0200;
    regs.x.bx = 0x0000;
    regs.x.dx = (25 << 8);
    int86(0x10, &regs, &regs); /* move cursor off-screen */
}
void txt80(void)
{
    union REGS regs;

    regs.x.ax = 3;
    int86(0x10, &regs, &regs);
}
void plot(unsigned int x, unsigned int y, unsigned char color)
{
    unsigned int pixaddr = (y * 80) + (x | 1);
    vidmem[pixaddr] = (vidmem[pixaddr] & 0x0F) | color;
}
int tile(int x, int y, int s, int t, int width, int height)
{
    unsigned char tileChar, borderColor, *tile;
    int ss, w;
    int pixaddr;

    pixaddr = y * 80 + (x << 1);
    if ((x <= CENTER_X) && ((x + width)  > CENTER_X)
     && (y <= CENTER_Y) && ((y + height) > CENTER_Y))
        borderColor = 0x0F;
    else
        borderColor = 0x0;
    if (s >= 0 && t >= 0 && (s >> 4) < mapWidth && (t >> 4) < mapHeight)
    {
        tile = tileMap[(t >> 4) * mapWidth + (s >> 4)];
        tileChar = 0xB1;
    }
    else
    {
        tile = NULL;
        tileChar = 0xDB;
    }
    if (tile)
    {
        s &= 0x0F;
        t &= 0x0F;
        tile   += t * 16 + s;
        while (height--)
        {
            ss = s;
            for (w = 0; w < width; w++)
            {
                tileChar = 0x20; // space
                if (t == 0 || t == 15)
                    tileChar = 0xC4;
                if (ss == 0)
                {
                    tileChar = 0xB3;
                    if (t == 0)
                        tileChar = 0xDA;
                    else if (t == 15)
                        tileChar = 0xC0;
                }
                else if (ss == 15)
                {
                    tileChar = 0xB3;
                    if (t == 0)
                        tileChar = 0xBF;
                    else if (t == 15)
                        tileChar = 0xD9;
                }
                vidmem[pixaddr + (w << 1)]     = tileChar;
                vidmem[pixaddr + (w << 1) + 1] = (tile[w] << 4) | borderColor;
                ss++;
            }
            t++;
            pixaddr = pixaddr + 80;
            tile   += 16;
        }
    }
    else
    {   // Emptry tile
        borderColor |= 0x80;
        while (height--)
        {
            for (w = 0; w < width; w++)
            {
                vidmem[pixaddr + (w << 1)]     = tileChar;
                vidmem[pixaddr + (w << 1) + 1] = borderColor;
            }
            pixaddr = pixaddr + 80;
        }

    }
}
void tileRow(int y, int s, int t, int height)
{
    int x;

    x = TILE_WIDTH - (s & (TILE_WIDTH-1));
    tile(0, y, s, t, x, height);
    s = (s + TILE_WIDTH) & ~(TILE_WIDTH-1);
    for (; SCREEN_WIDTH - x > TILE_WIDTH; x += TILE_WIDTH)
    {
        tile(x, y, s, t, TILE_WIDTH, height);
        s += TILE_WIDTH;
    }
    tile(x, y, s, t, SCREEN_WIDTH - x, height);
}
void tileScrn(int s, int t)
{
    int y;

    y = TILE_HEIGHT - (t & (TILE_HEIGHT-1));
    tileRow(0, s, t, y);
    t = (t + TILE_HEIGHT) & ~(TILE_HEIGHT-1);
    for (; SCREEN_HEIGHT - y > TILE_HEIGHT; y += TILE_HEIGHT)
    {
        tileRow(y, s, t, TILE_HEIGHT);
        t += TILE_HEIGHT;
    }
    tileRow(y, s, t, SCREEN_HEIGHT - y);
}
/*
 * Tile map editor
 */
int main(int argc, char **argv)
{
    int orgS, orgT, extS, extT, centerS, centerT;
    unsigned char *centerTile, tilePix;
    char quit, cycle;

    orgS = 0;
    orgT = 0;
    extS = mapWidth  << 4;
    extT = mapHeight << 4;
    quit = 0;
    buildmap();
    txt40();
    do
    {
        tileScrn(orgS, orgT);
        centerS = orgS + CENTER_X;
        centerT = orgT + CENTER_Y;
        centerTile = tileMap[(centerT >> 4) * mapWidth + (centerS >> 4)];
        if (centerTile)
            tilePix = centerTile[(centerT & 0x0F) * 16 + (centerS & 0x0F)] << 4;
        else
            tilePix = 0x00;
        vidmem[CENTER_Y * SCREEN_WIDTH * 2 + CENTER_X * 2] = 0xCE;
        while (!kbhit())
        {
            vidmem[CENTER_Y * SCREEN_WIDTH * 2 + CENTER_X * 2 + 1] = tilePix | (cycle++ & 0x0F);
        }
        switch(extgetch())
        {
            case UP_ARROW:
                if (centerT > 0)
                    orgT--;
                break;
            case DOWN_ARROW:
                if (centerT < extT - 1)
                    orgT++;
                break;
            case LEFT_ARROW:
                if (centerS > 0)
                    orgS--;
                break;
            case RIGHT_ARROW:
                if (centerS < extS - 1)
                    orgS++;
                break;
            case ESCAPE:
            case 'q':
            case 'Q':
                quit = 1;
                break;
        }
    } while (!quit);
    txt80();
    return 0;
}
