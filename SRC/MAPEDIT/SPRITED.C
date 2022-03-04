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
unsigned char far *vidptr;
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
            vidptr[y * 80 + (x << 1) + 0] = charlo;
            vidptr[y * 80 + (x << 1) + 1] = pixlo;
            vidptr[y * 80 + (x << 1) + 2] = charhi;
            vidptr[y * 80 + (x << 1) + 3] = pixhi;
        }
    }
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
    int currentIndex, plotX, plotY;
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
        printf("Usage: sprited <filebase>\n");
        exit(1);
    }
    txt40();
    vidptr        = vidmem + (SCREEN_HEIGHT - spriteHeight) / 2 * 80 + (SCREEN_WIDTH - spriteWidth);
    currentIndex  = 0;
    currentSprite = spritePage;
    currentColor  = 0x0F;
    plotX         = spriteWidth / 2;
    plotY         = spriteHeight / 2;
    quit          = 0;
    do
    {
        sprite40(currentSprite);
        gotoxy(0, 24);
        printf("Color:  ID:%d ", currentIndex);
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
        vidptr[plotY * SCREEN_WIDTH * 2 + plotX * 2] = 0xCE;
        while (!kbhit())
            vidptr[plotY * SCREEN_WIDTH * 2 + plotX * 2 + 1] = spritePix | (cycle++ & 0x0F);
        vidptr[plotY * SCREEN_WIDTH * 2 + plotX * 2] = ' ';
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
            case ' ': // Set tile pixel color
                if (plotX & 1)
                    currentSprite[(plotY * spriteWidth + plotX) >> 1] = (currentSprite[(plotY * spriteWidth + plotX) >> 1] & 0x0F) | (currentColor << 4);
                else
                    currentSprite[(plotY * spriteWidth + plotX) >> 1] = (currentSprite[(plotY * spriteWidth + plotX) >> 1] & 0xF0) | currentColor;
                modified = 1;
                break;
            case 'f': // Fill tile with current color
                modified = 1;
                break;
            case 'N': // Next color
                currentColor += 7;
            case 'n':
                currentColor = (currentColor + 1) & 0x0F;
                break;
            case 'c': // Copy current image
                break;
            case 'v': // Paste selected tile
                modified = 1;
                break;
            case 'x': // Exchange current and selected tile
                modified = 1;
                break;
            case 'a': // Add image
                modified = 1;
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
