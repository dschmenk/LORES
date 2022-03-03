#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <bios.h>
#include <dos.h>
#include <conio.h>
#include "lores.h"
#include "mapio.h"
#define CTRL_C          0x0003
#define CTRL_V          0x0016
#define CTRL_X          0x0018
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
char statusClear[] = "                                      ";
struct tile_t
{
    unsigned char tile[TILE_WIDTH * TILE_HEIGHT / 2];
    unsigned char tileExp[TILE_WIDTH * TILE_HEIGHT];
    int id;
    int refcount;
};
unsigned char far *vidmem = (char far *)0xB8000000L;
int mapWidth, mapHeight;
int tileCount;
struct tile_t far * far *tileMap;
struct tile_t far *tileSet;
void tileSetRebuild(void)
{
    int s, row, col;
    struct tile_t far *tileptr;

    tileptr = tileSet;
    for (s = 0; s < tileCount; s++)
    {
        for (row = 0; row < TILE_HEIGHT; row++)
            for (col = 0; col < TILE_WIDTH; col += 2)
                tileptr->tile[row * 8 + col / 2] = tileptr->tileExp[row * 16 + col] | (tileptr->tileExp[row * 16 + col + 1] << 4);
        tileptr++;
    }
}
void LoadMap(void)
{
    int i, row, col;
    unsigned long mapDim;
    struct tile_t far *tileptr, far * far *mapptr;

    tileCount = tilesetLoad("demo.set", (unsigned char far * *)&tileSet, sizeof(struct tile_t));
    mapDim    = tilemapLoad("demo.map", (unsigned char far *)tileSet, sizeof(struct tile_t), (unsigned char far * far * *)&tileMap);
    mapHeight = mapDim >> 16;
    mapWidth  = mapDim;
    tileptr   = tileSet;
    for (i = 0; i < tileCount; i++)
    {
        tileptr->id = i;
        tileptr->refcount = 0;
        for (row = 0; row < TILE_HEIGHT; row++)
            for (col = 0; col < TILE_WIDTH; col += 2)
            {
                tileptr->tileExp[row * 16 + col]     = tileptr->tile[row * 8 + col / 2] & 0x0F;
                tileptr->tileExp[row * 16 + col + 1] = tileptr->tile[row * 8 + col / 2] >> 4;
            }
        tileptr++;
    }
    mapptr = tileMap;
    for (i = 0; i < mapHeight * mapWidth; i++)
        (*mapptr++)->refcount++;
}
#if 0
#define extgetch()  _bios_keybrd(_KEYBRD_READ)
#else
unsigned short extgetch(void)
{
    unsigned short extch;

    extch = getch();
    if (!extch)
        extch = getch() << 8;
    return extch;
}
#endif
void gotoxy(int x, int y)
{
    union REGS regs;

    regs.x.ax = 0x0200;
    regs.x.bx = 0x0000;
    regs.x.dx = (y << 8)| x;
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
    unsigned int pixaddr = (y * 80) + (x * 2) | 1;
    vidmem[pixaddr] = (color << 4) | color;
}
void tile40(int x, int y, int s, int t, int width, int height)
{
    unsigned char tileChar, borderColor, far *tileptr;
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
        if (tileMap[(t >> 4) * mapWidth + (s >> 4)])
            tileptr = (tileMap[(t >> 4) * mapWidth + (s >> 4)])->tileExp;
        else
            tileptr = NULL;
        tileChar = 0xB1;
    }
    else
    {
        tileptr = NULL;
        tileChar = 0xDB;
    }
    if (tileptr)
    {
        s &= 0x0F;
        t &= 0x0F;
        tileptr   += t * 16 + s;
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
                vidmem[pixaddr + (w << 1) + 1] = (tileptr[w] << 4) | borderColor;
                ss++;
            }
            t++;
            pixaddr = pixaddr + 80;
            tileptr += 16;
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
void tileRow40(int y, int s, int t, int height)
{
    int x;

    x = TILE_WIDTH - (s & (TILE_WIDTH-1));
    tile40(0, y, s, t, x, height);
    s = (s + TILE_WIDTH) & ~(TILE_WIDTH-1);
    for (; SCREEN_WIDTH - x > TILE_WIDTH; x += TILE_WIDTH)
    {
        tile40(x, y, s, t, TILE_WIDTH, height);
        s += TILE_WIDTH;
    }
    tile40(x, y, s, t, SCREEN_WIDTH - x, height);
}
void tileScrn40(int s, int t)
{
    int y;

    y = TILE_HEIGHT - (t & (TILE_HEIGHT-1));
    tileRow40(0, s, t, y);
    t = (t + TILE_HEIGHT) & ~(TILE_HEIGHT-1);
    for (; SCREEN_HEIGHT - y > TILE_HEIGHT; y += TILE_HEIGHT)
    {
        tileRow40(y, s, t, TILE_HEIGHT);
        t += TILE_HEIGHT;
    }
    tileRow40(y, s, t, SCREEN_HEIGHT - y);
}
struct tile_t far * tileSelectList(int s, int t)
{
    struct tile_t far * far *saveMap;
    int saveWidth, saveHeight;
    int selected, quit;

    /*
     * Re-use tileScrn to select tile from tile set
     */
    plot40(6, 24, BLACK);
    gotoxy(0, 24);
    printf(statusClear);
    fflush(stdout);
    saveWidth  = mapWidth;
    saveHeight = mapHeight;
    saveMap    = tileMap;
    mapWidth   = tileCount;
    mapHeight  = 1;
    tileMap    = (struct tile_t far * far *)_fmalloc(tileCount * sizeof(struct tile_t far *));
    for (selected = 0; selected < tileCount; selected++)
        tileMap[selected] = &tileSet[selected];
    selected = saveMap[t * saveWidth + s]->id;
    quit     = 0;
    do
    {
        tileScrn40(((selected) << 4) + TILE_WIDTH/2 - CENTER_X, TILE_HEIGHT/2 - CENTER_Y);
        gotoxy(0, 24);
        printf("    Select Tile ID:%d, ESC Cancels  ", tileSet[selected].id);
        gotoxy(0, 25);
        switch(extgetch())
        {
            case LEFT_ARROW: // Move left
                if (selected > 0)
                    selected--;
                break;
            case RIGHT_ARROW: // Move right
                if (selected < tileCount - 1)
                    selected++;
                break;
            case ESCAPE:
                selected = saveMap[t * saveWidth + s]->id;
            case ' ':
                quit = 1;
                break;
        }
    } while (!quit);
    _ffree(tileMap);
    tileMap    = saveMap;
    mapWidth   = saveWidth;
    mapHeight  = saveHeight;
    gotoxy(0, 24);
    printf(statusClear);
    fflush(stdout);
    return &tileSet[selected];
}
struct tile_t far * tileNew(void)
{
    int i, j;
    struct tile_t far *newSet;
    newSet = (struct tile_t far *)_fmalloc((tileCount + 1) * sizeof(struct tile_t));
    if (newSet)
    {
        /*
         * Init fresh tile
         */
        newSet[tileCount].id = tileCount;
        for (j = 0; j < TILE_WIDTH * TILE_HEIGHT; j++)
            newSet[tileCount].tileExp[j] = GREY;
        /*
         * Copy over existing tiles
         */
        for (i = 0; i < tileCount; i++)
        {
            newSet[i].id = i;
            for (j = 0; j < TILE_WIDTH * TILE_HEIGHT; j++)
                newSet[i].tileExp[j] = tileSet[i].tileExp[j];
        }
        /*
         * Update map references
         */
        for (j = 0; j < mapHeight; j++)
            for (i = 0; i < mapWidth; i++)
                tileMap[j * mapWidth + i] = &newSet[(tileMap[j * mapWidth + i])->id];
        _ffree(tileSet);
        tileSet = newSet;
        return &tileSet[tileCount++];
    }
    return NULL;
}
void mapPreview(void)
{
    int s, t, maxS, maxT, quit;
    tileSetRebuild();
    quit =
    s    =
    t    = 0;
    maxS = mapWidth*TILE_WIDTH   - 160;
    maxT = mapHeight*TILE_HEIGHT - 100;
    viewInit(gr160(BLACK, BLACK), 0, 6, mapWidth, mapHeight, (unsigned char far * far *)tileMap);
    do
    {
        tileScrn(s, t);
        switch(extgetch())
        {
            case UP_ARROW: // Move up
                t -= TILE_HEIGHT;
                if (t < 0)
                    t = 0;
                break;
            case DOWN_ARROW: // Move down
                t += TILE_HEIGHT;
                if (t > maxT)
                    t = maxT;
                break;
            case LEFT_ARROW: // Move left
                s -= TILE_WIDTH;
                if (s < 0)
                    s = 0;
                break;
            case RIGHT_ARROW: // Move right
                s += TILE_WIDTH;
                if (s > maxS)
                    s = maxS;
                break;
            case ESCAPE:
                quit = 1;
                break;
        }
    } while (!quit);
    viewExit();
    txt40();
}
char getRowCol(char *action)
{
    unsigned short ch;

    plot40(6, 24, BLACK);
    gotoxy(0, 24);
    printf(statusClear);
    gotoxy(8, 24);
    printf("%s R)ow or C)olumn?", action);
    fflush(stdout);
    ch = toupper(extgetch());
    gotoxy(0, 24);
    printf(statusClear);
    return ch;
}
void rowNew(int row)
{
    int i, j;
    struct tile_t far * far *newMap;

    newMap = (struct tile_t far * far *)_fmalloc((mapHeight + 1) * mapWidth * sizeof(unsigned char far *));
    if (newMap)
    {
        for (j = 0; j <= row; j++)
            for (i = 0; i < mapWidth; i++)
                newMap[j * mapWidth + i] = tileMap[j * mapWidth + i];
        for (j = row; j < mapHeight; j++)
            for (i = 0; i < mapWidth; i++)
                newMap[(j + 1) * mapWidth + i] = tileMap[j * mapWidth + i];
        mapHeight++;
        _ffree(tileMap);
        tileMap = newMap;
    }
}
void colNew(int col)
{
    int i, j;
    struct tile_t far * far *newMap;

    newMap = (struct tile_t far * far *)_fmalloc(mapHeight * (mapWidth + 1) * sizeof(unsigned char far *));
    if (newMap)
    {
        for (i = 0; i <= col; i++)
            for (j = 0; j < mapHeight; j++)
                newMap[j * (mapWidth + 1) + i] = tileMap[j * mapWidth + i];
        for (i = col; i < mapWidth; i++)
            for (j = 0; j < mapHeight; j++)
                newMap[j * (mapWidth + 1) + (i + 1)] = tileMap[j * mapWidth + i];
        mapWidth++;
        _ffree(tileMap);
        tileMap = newMap;
    }
}
void rowDel(int row)
{
    int i, j;
    struct tile_t far * far *newMap;

    if ((mapHeight - 1) >= SCREEN_HEIGHT / TILE_HEIGHT)
    {
        newMap = (struct tile_t far * far *)_fmalloc((mapHeight - 1) * mapWidth * sizeof(unsigned char far *));
        if (newMap)
        {
            for (j = 0; j < row; j++)
                for (i = 0; i < mapWidth; i++)
                    newMap[j * mapWidth + i] = tileMap[j * mapWidth + i];
            for (j = row + 1; j < mapHeight; j++)
                for (i = 0; i < mapWidth; i++)
                    newMap[(j - 1) * mapWidth + i] = tileMap[j * mapWidth + i];
            mapHeight--;
            _ffree(tileMap);
            tileMap = newMap;
        }
    }
}
void colDel(int col)
{
    int i, j;
    struct tile_t far * far *newMap;

    if ((mapWidth - 1) >= SCREEN_WIDTH / TILE_WIDTH)
    {
        newMap = (struct tile_t far * far *)_fmalloc(mapHeight * (mapWidth - 1) * sizeof(unsigned char far *));
        if (newMap)
        {
            for (i = 0; i < col; i++)
                for (j = 0; j < mapHeight; j++)
                    newMap[j * (mapWidth - 1) + i] = tileMap[j * mapWidth + i];
            for (i = col + 1; i < mapWidth; i++)
                for (j = 0; j < mapHeight; j++)
                    newMap[j * (mapWidth - 1) + (i - 1)] = tileMap[j * mapWidth + i];
            mapWidth--;
            _ffree(tileMap);
            tileMap = newMap;
        }
    }
}
/*
 * Tile map editor
 */
int main(int argc, char **argv)
{
    int orgS, orgT, extS, extT, centerS, centerT, i;
    struct tile_t far *centerTile, far *selectTile;
    unsigned char tilePix, currentColor;
    char quit, cycle, statusLine[40];

    LoadMap();
    txt40();
    orgS = 0;
    orgT = 0;
    extS = mapWidth  << 4;
    extT = mapHeight << 4;
    quit = 0;
    currentColor = 0x0F;
    selectTile = NULL;
    do
    {
        tileScrn40(orgS, orgT);
        centerS = orgS + CENTER_X;
        centerT = orgT + CENTER_Y;
        centerTile = tileMap[(centerT >> 4) * mapWidth + (centerS >> 4)];
        if (centerTile)
            tilePix = centerTile->tileExp[(centerT & 0x0F) * 16 + (centerS & 0x0F)] << 4;
        else
            tilePix = 0x00;
        vidmem[CENTER_Y * SCREEN_WIDTH * 2 + CENTER_X * 2] = 0xCE;
        gotoxy(0, 24);
        printf("Color:  ID:%d (%d) (%d, %d) ", centerTile ? centerTile->id : -1, selectTile ? selectTile->id : -1, centerS >> 4, centerT >> 4);
        gotoxy(0, 25);
        plot40(6, 24, currentColor);
        while (!kbhit())
        {
            vidmem[CENTER_Y * SCREEN_WIDTH * 2 + CENTER_X * 2 + 1] = tilePix | (cycle++ & 0x0F);
        }
        switch(extgetch())
        {
            case UP_ARROW: // Move up
                if (centerT > 0)
                    orgT--;
                break;
            case DOWN_ARROW: // Move down
                if (centerT < extT - 1)
                    orgT++;
                break;
            case LEFT_ARROW: // Move left
                if (centerS > 0)
                    orgS--;
                break;
            case RIGHT_ARROW: // Move right
                if (centerS < extS - 1)
                    orgS++;
                break;
            case ' ': // Set tile pixel color
                if (centerTile)
                    centerTile->tileExp[(centerT & (TILE_HEIGHT-1)) * TILE_HEIGHT + (centerS & (TILE_WIDTH-1))] = currentColor;
                break;
            case 'F': // Fill tile with current color
            case 'f':
                if (centerTile)
                    for (i = 0; i < TILE_WIDTH * TILE_HEIGHT; i++)
                        centerTile->tileExp[i] = currentColor;
                break;
            case 'N': // Next color
            case 'n':
                currentColor = (currentColor + 1) & 0x0F;
                break;
            case 'C': // Copy current tile
            case 'c':
                selectTile = centerTile;
                break;
            case 'V': // Paste selected tile
            case 'v':
                tileMap[(centerT >> 4) * mapWidth + (centerS >> 4)] = selectTile;
                break;
            case 'X': // Exchange current and selected tile
            case 'x':
                tileMap[(centerT >> 4) * mapWidth + (centerS >> 4)] = selectTile;
                selectTile = centerTile;
                break;
            case 'S': // Select tile from list
            case 's':
                selectTile = tileSelectList(centerS >> 4, centerT >> 4);
                break;
            case 'P': // Preview
            case 'p':
                mapPreview();
                break;
            case 'T': // New tile
            case 't':
                selectTile = tileNew();
                break;
            case 'I': // Insert row/column
            case 'i':
                switch(getRowCol("Insert"))
                {
                    case 'R':
                        rowNew(centerT >> 4);
                        break;
                    case 'C':
                        colNew(centerS >> 4);
                        break;
                }
                extS = mapWidth  << 4;
                extT = mapHeight << 4;
                break;
            case 'D': // Delete row/column
            case 'd':
                switch(getRowCol("Delete"))
                {
                    case 'R':
                        rowDel(centerT >> 4);
                        break;
                    case 'C':
                        colDel(centerS >> 4);
                        break;
                }
                extS = mapWidth  << 4;
                extT = mapHeight << 4;
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
