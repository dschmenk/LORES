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
#define PAGE_UP         0x4900
#define PAGE_DOWN       0x5100
#define HOME            0x4700
#define END             0x4F00
#define SCREEN_WIDTH    40
#define SCREEN_HEIGHT   24
#define STATUS_LINE     SCREEN_HEIGHT
#define CENTER_X        (SCREEN_WIDTH/2)
#define CENTER_Y        (SCREEN_HEIGHT/2)
#define MAX_SPRITE_WIDTH    40
#define MAX_SPRITE_HEIGHT   24
#define MAX_NUM_SPRITES     64
char statusClear[] = "                                      ";
unsigned char far *vidmem = (char far *)0xB8000000L;
unsigned char far *disptr;
int spriteWidth, spriteHeight, spriteCount, sizeofSprite;
unsigned char far *spritePage;
unsigned short extgetch(void)
{
    unsigned short extch;

    extch = getch();
    if (!extch)
        extch = getch() << 8;
    return extch;
}
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
int spriteNew(void)
{
    int i, j;
    unsigned char far *newPage, spriteptr;

    newPage = (unsigned char far *)_fmalloc((spriteCount + 1) * sizeofSprite);
    if (newPage)
    {
        /*
         * Init fresh tile
         */
        for (j = 0; j < sizeofSprite; j++)
            newPage[spriteCount * sizeofSprite + j] = GREY | (GREY << 4);
        /*
         * Copy over existing tiles
         */
        for (i = 0; i < spriteCount; i++)
            for (j = 0; j < sizeofSprite; j++)
                newPage[i * sizeofSprite + j] = spritePage[i * sizeofSprite + j];
        if (spritePage)
            _ffree(spritePage);
        spritePage = newPage;
        return ++spriteCount;
    }
    return spriteCount;
}
int spriteDel(int index)
{
    int i, j;
    unsigned char far *newPage, spriteptr;

    newPage = (unsigned char far *)_fmalloc((spriteCount - 1) * sizeofSprite);
    if (newPage)
    {
        /*
         * Copy over existing tiles
         */
        for (i = 0; i < index; i++)
            for (j = 0; j < sizeofSprite; j++)
                newPage[i * sizeofSprite + j] = spritePage[i * sizeofSprite + j];
        for (i = index + 1; i < spriteCount; i++)
            for (j = 0; j < sizeofSprite; j++)
                newPage[(i - 1) * sizeofSprite + j] = spritePage[i * sizeofSprite + j];
        if (spritePage)
            _ffree(spritePage);
        spritePage = newPage;
        if (index >= --spriteCount)
            index = spriteCount - 1;
    }
    return index;
}
void rowNew(int row)
{
    int i, j, k, newSize;
    unsigned char far * newPage;

    if (spriteHeight + 1 <= SCREEN_HEIGHT)
    {
        newSize = spriteWidth * (spriteHeight + 1)/2;
        newPage = (unsigned char far *)_fmalloc(spriteCount * newSize);
        if (newPage)
        {
            for (k = 0; k < spriteCount; k++)
            {
                for (j = 0; j <= row; j++)
                    for (i = 0; i < spriteWidth; i += 2)
                        newPage[k * newSize + (j * spriteWidth + i)/2] = spritePage[k * sizeofSprite + (j * spriteWidth + i)/2];
                for (j = row; j < spriteHeight; j++)
                    for (i = 0; i < spriteWidth; i += 2)
                        newPage[k * newSize + ((j + 1) * spriteWidth + i)/2] = spritePage[k * sizeofSprite + (j * spriteWidth + i)/2];
            }
            _ffree(spritePage);
            spritePage   = newPage;
            sizeofSprite = newSize;
            spriteHeight++;
        }
    }
}
void colNew(int col)
{
    int i, j, k, newWidth, newSize;
    unsigned char far * newPage;

    if (spriteWidth + 2 <= SCREEN_WIDTH)
    {
        col     &= ~1;
        newWidth = spriteWidth + 2;
        newSize  = newWidth * spriteHeight / 2;
        newPage  = (unsigned char far *)_fmalloc(spriteCount * newSize);
        if (newPage)
        {
            for (k = 0; k < spriteCount; k++)
            {
                for (i = 0; i <= col; i += 2)
                    for (j = 0; j < spriteHeight; j++)
                        newPage[k * newSize + (j * newWidth + i)/2] = spritePage[k * sizeofSprite + (j * spriteWidth + i)/2];
                for (i = col; i < spriteWidth; i += 2)
                    for (j = 0; j < spriteHeight; j++)
                        newPage[k * newSize + (j * newWidth + (i + 2))/2] = spritePage[k * sizeofSprite + (j * spriteWidth + i)/2];
            }
            _ffree(spritePage);
            spritePage   = newPage;
            sizeofSprite = newSize;
            spriteWidth  = newWidth;
        }
    }
}
int rowDel(int row)
{
    int i, j, k, newSize;
    unsigned char far * newPage;

    if (spriteHeight > 1)
    {
        newSize = spriteWidth * (spriteHeight - 1)/2;
        newPage = (unsigned char far *)_fmalloc(spriteCount * newSize);
        if (newPage)
        {
            for (k = 0; k < spriteCount; k++)
            {
                for (j = 0; j < row; j++)
                    for (i = 0; i < spriteWidth; i += 2)
                        newPage[k * newSize + (j * spriteWidth + i)/2] = spritePage[k * sizeofSprite + (j * spriteWidth + i)/2];
                for (j = row + 1; j < spriteHeight; j++)
                    for (i = 0; i < spriteWidth; i += 2)
                        newPage[k * newSize + ((j - 1) * spriteWidth + i)/2] = spritePage[k * sizeofSprite + (j * spriteWidth + i)/2];
            }
            _ffree(spritePage);
            spritePage   = newPage;
            sizeofSprite = newSize;
            spriteHeight--;
            if (row >= spriteHeight)
                row--;
        }
    }
    return row;
}
int colDel(int col)
{
    int i, j, k, newWidth, newSize;
    unsigned char far * newPage;

    if (spriteWidth > 2)
    {
        col     &= ~1;
        newWidth = spriteWidth - 2;
        newSize  = newWidth * spriteHeight / 2;
        newPage  = (unsigned char far *)_fmalloc(spriteCount * newSize);
        if (newPage)
        {
            for (k = 0; k < spriteCount; k++)
            {
                for (i = 0; i < col; i += 2)
                    for (j = 0; j < spriteHeight; j++)
                        newPage[k * newSize + (j * newWidth + i)/2] = spritePage[k * sizeofSprite + (j * spriteWidth + i)/2];
                for (i = col + 2; i < spriteWidth; i += 2)
                    for (j = 0; j < spriteHeight; j++)
                        newPage[k * newSize + (j * newWidth + (i - 2))/2] = spritePage[k * sizeofSprite + (j * spriteWidth + i)/2];
            }
            _ffree(spritePage);
            spritePage   = newPage;
            sizeofSprite = newSize;
            spriteWidth  = newWidth;
            if (col >= spriteWidth)
                col -= 2;
        }
    }
    return col;
}
char getResponse(char *question)
{
    unsigned short ch;

    plot40(6, 24, BLACK);
    gotoxy(0, 24);
    printf(statusClear);
    gotoxy(8, 24);
    printf("%s", question);
    fflush(stdout);
    ch = toupper(extgetch());
    gotoxy(0, 24);
    printf(statusClear);
    return ch;
}
void LoadSprite(char *filebase)
{
    char filename[128];

    strcpy(filename, filebase);
    strcat(filename, ".spr");
    spriteCount  = spriteLoad(filename, (unsigned char far * *)&spritePage, &spriteWidth, &spriteHeight);
    sizeofSprite = spriteWidth * spriteHeight / 2;
}
void SaveSprite(char *filebase)
{
    char filename[128];

    strcpy(filename, filebase);
    strcat(filename, ".spr");
    if (spriteSave(filename, (unsigned char far *)spritePage, spriteWidth, spriteHeight, spriteCount))
    {
        plot40(6, 24, BLACK);
        gotoxy(0, 24);
        printf(statusClear);
        gotoxy(8, 24);
        printf("Saving file %s failed. Press a key.", filename);
        getch();
    }
}
/*
 * Tile map editor
 */
int main(int argc, char **argv)
{
    unsigned char far *currentSprite;
    int currentIndex, selectIndex, plotX, plotY, j;
    unsigned char currentColor, spritePix;
    char quit, modified, cycle, spriteName[40];

    if (argc > 1)
    {
        strcpy(spriteName, argv[1]);
        LoadSprite(spriteName);
        modified = 0;
    }
    else
    {
        strcpy(spriteName, "noname");
        spriteCount  = 0;
        spriteWidth  =
        spriteHeight = 8;
        sizeofSprite = spriteWidth * spriteHeight / 2;
        spritePage   = NULL;
        modified     = spriteNew();
    }
    txt40();
    disptr        = vidmem + (SCREEN_HEIGHT - spriteHeight) / 2 * 80 + (SCREEN_WIDTH - spriteWidth);
    selectIndex   =
    currentIndex  = 0;
    currentSprite = spritePage;
    currentColor  = 0x0F;
    plotX         = spriteWidth  / 2;
    plotY         = spriteHeight / 2;
    quit          = 0;
    do
    {
        sprite40(currentSprite);
        gotoxy(0, 24);
        printf("Color:  ID:%d (%d)", currentIndex, selectIndex);
        gotoxy(0, 25);
        plot40(6, 24, currentColor);
        if (plotX < 0)
            plotX = 0;
        else if (plotX >= spriteWidth)
            plotX = spriteWidth - 1;
        if (plotY < 0)
            plotY = 0;
        else if (plotY >= spriteHeight)
            plotY = spriteHeight - 1;
        if (plotX & 1)
            spritePix = currentSprite[(plotY * spriteWidth + plotX) >> 1] & 0xF0;
        else
            spritePix = currentSprite[(plotY * spriteWidth + plotX) >> 1] << 4;
        disptr[plotY * SCREEN_WIDTH * 2 + plotX * 2] = 0xCE;
        while (!kbhit())
            disptr[plotY * SCREEN_WIDTH * 2 + plotX * 2 + 1] = spritePix | (cycle++ & 0x0F);
        disptr[plotY * SCREEN_WIDTH * 2 + plotX * 2] = ' ';
        switch (extgetch())
        {
            case UP_ARROW: // Move cursor up
                plotY--;
                break;
            case DOWN_ARROW: // Move cursor down
                plotY++;
                break;
            case '4': // Previous sprite image
            case HOME:
                if (currentIndex)
                {
                    currentIndex--;
                    currentSprite = spritePage + currentIndex * sizeofSprite;
                }
                break;
            case LEFT_ARROW: // Move cursor left
                plotX--;
                break;
            case '6': // Next sprite image
            case END:
                if (currentIndex < spriteCount - 1)
                {
                    currentIndex++;
                    currentSprite = spritePage + currentIndex * sizeofSprite;
                }
                break;
            case RIGHT_ARROW: // Move cursor right
                plotX++;
                break;
            case ' ': // Set sprite pixel color
                if (plotX & 1)
                    currentSprite[(plotY * spriteWidth + plotX) >> 1] = (currentSprite[(plotY * spriteWidth + plotX) >> 1] & 0x0F) | (currentColor << 4);
                else
                    currentSprite[(plotY * spriteWidth + plotX) >> 1] = (currentSprite[(plotY * spriteWidth + plotX) >> 1] & 0xF0) | currentColor;
                modified = 1;
                break;
            case 'f': // Fill sprite with current color
                for (j = 0; j < sizeofSprite; j++)
                    currentSprite[j] = currentColor | (currentColor << 4);
                modified = 1;
                break;
            case 'N': // Next color
                currentColor += 7;
            case 'n':
                currentColor = (currentColor + 1) & 0x0F;
                break;
            case 's': // New sprite
                currentIndex  = spriteNew() - 1;
                currentSprite = spritePage + currentIndex * sizeofSprite;
                modified = 1;
                break;
            case 'K': // Kill sprite
                if (spriteCount > 1 && getResponse("Delete sprite image (Y/N)?") == 'Y')
                {
                    currentIndex  = spriteDel(currentIndex);
                    currentSprite = spritePage + currentIndex * sizeofSprite;
                    modified = 1;
                }
                break;
            case 'c': // Copy current image
                selectIndex = currentIndex;
                break;
            case 'v': // Paste selected tile
                for (j = 0; j < sizeofSprite; j++)
                    currentSprite[j] = spritePage[selectIndex * sizeofSprite + j];
                modified = 1;
                break;
            case 'x': // Exchange current and selected tile
                for (j = 0; j < sizeofSprite; j++)
                {
                    spritePix = currentSprite[j];
                    currentSprite[j] = spritePage[selectIndex * sizeofSprite + j];
                    spritePage[selectIndex * sizeofSprite + j] = spritePix;
                }
                modified = 1;
                break;
            case 'i': // Insert row/column
                switch (getResponse("Insert R)ow or C)olumn?"))
                {
                    case 'R':
                        rowNew(plotY);
                        modified = 1;
                        break;
                    case 'C':
                        colNew(plotX);
                        modified = 1;
                        break;
                }
                disptr = vidmem + (SCREEN_HEIGHT - spriteHeight) / 2 * 80 + (SCREEN_WIDTH - spriteWidth);
                currentSprite = spritePage + currentIndex * sizeofSprite;
                break;
            case 'd': // Delete row/column
                switch(getResponse("Delete R)ow or C)olumn?"))
                {
                    case 'R':
                        plotY = rowDel(plotY);
                        modified = 1;
                        break;
                    case 'C':
                        plotX = colDel(plotX);
                        modified = 1;
                        break;
                }
                txt40(); // Clear screen
                disptr = vidmem + (SCREEN_HEIGHT - spriteHeight) / 2 * 80 + (SCREEN_WIDTH - spriteWidth);
                currentSprite = spritePage + currentIndex * sizeofSprite;
                break;
            case 'W': // Write sprite page to file
                plot40(6, 24, BLACK);
                gotoxy(0, 24);
                printf(statusClear);
                gotoxy(8, 24);
                printf("Write sprite name:");
                fflush(stdout);
                gets(spriteName);
                gotoxy(0, 24);
                printf(statusClear);
            case 'w':
                SaveSprite(spriteName);
                modified = 0;
                break;
            case ESCAPE: // Quit
            case 'q':
                quit = 1;
                if (modified && getResponse("Write sprite (Y/N)?") == 'Y')
                    SaveSprite(spriteName);
                break;
        }
    } while (!quit);
    txt80();
    return 0;
}
