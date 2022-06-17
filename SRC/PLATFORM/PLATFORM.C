#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <conio.h>
#include "lores.h"
#include "tiler.h"
#include "mapio.h"
#include "keyboard.h"
/*
 * PPI stands for Programmable Peripheral Interface (which is the Intel 8255A chip)
 * The PPI ports are only for IBM PC and XT, however port A is mapped to the same
 * I/O address as the Keyboard Controller's (Intel 8042 chip) output buffer for compatibility.
 */
#define PPI_PORT_A              0x60
#define PPI_PORT_B              0x61
#define PPI_PORT_C              0x62
#define PPI_COMMAND_REGISTER    0x63
/*
 * 8253 Programmable Interval Timer ports and registers
 * The clock input is a 1.19318 MHz signal
 */
#define CLOCK_FREQ              1193180L
#define PIT_COMMAND_REGISTER    0x43
#define PIT_COUNT_REGISTER      0x42

#define SET_SOUND(cnt)          do{outp(PIT_COMMAND_REGISTER, 0xB6);  \
                                 outp(PIT_COUNT_REGISTER, (cnt));   \
                                 outp(PIT_COUNT_REGISTER, ((cnt)>>8));}while(0)
#define START_SOUND             do{outp(PPI_PORT_B, inp(PPI_PORT_B)|0x03);}while(0)
#define STOP_SOUND               outp(PPI_PORT_B, inp(PPI_PORT_B)&0xFC)
/*
 * Screen dimensions
 */
#define SCREEN_WIDTH            160
#define SCREEN_HEIGHT           100
/*
 * Sprites
 */
#define HERO_SPRITE             0
#define HERO_IDLE               0
#define HERO_JUMP_LEFT          2
#define HERO_JUMP_RIGHT         3
#define HERO_RUN_LEFT           4
#define HERO_RUN_RIGHT          8
#define HERO_CLIMB              12
/*
 * Tile map
 */
#define TILE_AIR                0
#define TILE_GROUND             1
#define TILE_LADDER             2
#define TILE_SOLID              4
int mapWidth, mapHeight;
struct tile
{
    unsigned char tilePix[16*16/2];
    unsigned char tileFlags;
} far *tileset, far * far *tilemap;
int tileCount;
/*
 * Sprites
 */
unsigned char far *hero;
unsigned int heroWidth, heroHeight, sizeofHero;

int main(int argc, char **argv)
{
    unsigned long st;
    signed   char quit;
    unsigned char far *heroImage, far *heroPrevImage;
    struct tile far *tileAt, far *tileOn;
    int heroS, heroT, heroPrevS, heroPrevT, heroMapX, heroMapY;
    int heroVelS, heroVelT, heroOfstS, heroOfstT;

    if (!spriteLoad("hero.spr", &hero, &heroWidth, &heroHeight))
    {
        fprintf(stderr, "Unable to load hero sprite page\n");
        exit(1);
    }
    sizeofHero = heroWidth * heroHeight / 2;
    heroOfstS  = heroWidth/2;
    heroOfstT  = heroHeight - 1;
    if ((tileCount = tilesetLoad("world.set", (unsigned char far * *)&tileset, sizeof(struct tile))) == 0)
    {
        fprintf(stderr, "Unable to load tile set\n");
        exit(1);
    }
    /*
     * Flag tiles
     */
    tileset[0].tileFlags = TILE_AIR;
    tileset[1].tileFlags = TILE_GROUND;
    tileset[2].tileFlags = TILE_GROUND|TILE_LADDER;
    tileset[3].tileFlags = TILE_LADDER;
    st = tilemapLoad("world.map", (unsigned char far *)tileset, sizeof(struct tile), (unsigned char far * far * *)&tilemap);
    if (!st)
    {
        fprintf(stderr, "Unable to load tile map\n");
        exit(1);
    }
    mapWidth        = st;
    mapHeight       = st >> 16;
    quit            = 0;
    heroVelS        =
    heroVelT        = 0;
    heroS           = SCREEN_WIDTH/2;
    heroT           = 75;
    heroImage       =
    heroPrevImage   = hero;
    keyboardInstallDriver();
    viewInit(gr160(BLUE, BLUE), 0, 0, mapWidth, mapHeight, (unsigned char far * far *)tilemap);
    spriteEnable(HERO_SPRITE, heroS + heroWidth/2, heroT - heroHeight, heroWidth, heroHeight, hero);
    do
    {
#ifdef PROFILE
        rasterBorder(WHITE);
#endif
        heroPrevS     = heroS;
        heroPrevT     = heroT;
        heroMapX      = heroS >> 4;
        heroMapY      = heroT >> 4;
        tileAt        = tilemap[heroMapY * mapWidth + heroMapX];
        tileOn        = tilemap[(heroMapY + 1) * mapWidth + heroMapX];
        heroPrevImage = heroImage;
        /*
         * Hero movement input
         */
        if ((heroT & 0x000F) == 0x0F)
        {
            if (tileOn->tileFlags == TILE_AIR) // Fall
            {
                heroS    += heroVelS;
                heroT    += heroVelT >> 2;
                heroImage = hero + (heroVelS == 1 ? HERO_JUMP_RIGHT : HERO_JUMP_LEFT) * sizeofHero;
                heroVelT++;
            }
            else
            {
                heroVelS =
                heroVelT = 0;
                if ((tileAt->tileFlags & TILE_LADDER) && ((keyboardGetKey(SCAN_KP_2) || keyboardGetKey(SCAN_UP_ARROW)))) // Climb up
                {
                    heroS = (heroS & 0xFFF0) | 0x8;
                    heroT--;
                    heroImage = hero + (HERO_CLIMB + ((heroT >> 2) & 0x03)) * sizeofHero;
                }
                if ((tileOn->tileFlags & TILE_LADDER) && ((keyboardGetKey(SCAN_KP_8) || keyboardGetKey(SCAN_DOWN_ARROW)))) // Climb down
                {
                    heroS = (heroS & 0xFFF0) | 0x8;
                    heroT++;
                    heroImage = hero + (HERO_CLIMB + ((heroT >> 2) & 0x03)) * sizeofHero;
                }
                if (tileOn->tileFlags & TILE_GROUND)
                {
                    if (keyboardGetKey(SCAN_KP_4) || keyboardGetKey(SCAN_LEFT_ARROW)) // Move left
                    {
                        heroVelS = -1;
                        heroS--;
                        heroImage = hero + (HERO_RUN_LEFT + ((heroS >> 3) & 0x03)) * sizeofHero;
                    }
                    if (keyboardGetKey(SCAN_KP_6) || keyboardGetKey(SCAN_RIGHT_ARROW)) // Move right
                    {
                        heroVelS = 1;
                        heroS++;
                        heroImage = hero + (HERO_RUN_RIGHT + ((heroS >> 3) & 0x03)) * sizeofHero;
                    }
                }
            }
        }
        else
        {
            if (tileAt->tileFlags & TILE_LADDER)
            {
                if (keyboardGetKey(SCAN_KP_2) || keyboardGetKey(SCAN_UP_ARROW)) // Climb up
                {
                    heroS = (heroS & 0xFFF0) | 0x8;
                    heroT--;
                    heroImage = hero + (HERO_CLIMB + ((heroT >> 3) & 0x03)) * sizeofHero;
                }
                if (keyboardGetKey(SCAN_KP_8) || keyboardGetKey(SCAN_DOWN_ARROW)) // Climb down
                {
                    heroS = (heroS & 0xFFF0) | 0x8;
                    heroT++;
                    heroImage = hero + (HERO_CLIMB + ((heroT >> 3) & 0x03)) * sizeofHero;
                }
            }
            else if (tileAt->tileFlags & TILE_GROUND)
            {
                heroT     = (heroT & 0xFFF0) - 1;
                heroImage = hero;
                heroVelT  = 0;
            }
            else //if (tilemap[(heroMapY + 1) * mapWidth + heroMapX]->tileFlags & TILE_AIR) // Fall
            {
                heroS    += heroVelS;
                heroT    += heroVelT >> 2;
                heroImage = hero + (heroVelS == 1 ? HERO_JUMP_RIGHT : HERO_JUMP_LEFT) * sizeofHero;
                heroVelT++;
            }
        }
        if (heroS == heroPrevS && heroT == heroPrevT)
        {
            if ((heroT & 0x000F) == 0x0F)
                heroImage = hero + (HERO_IDLE + ((frameCount >> 6) & 0x01)) * sizeofHero;
        }
        else
        {
            st = spritePosition(HERO_SPRITE, heroS - heroOfstS, heroT - heroOfstT);
            heroS = st + heroOfstS;
            heroT = (st >> 16) + heroOfstT;
        }
        if (heroImage != heroPrevImage)
            spriteUpdate(HERO_SPRITE, heroImage);
        if (!quit && keyboardGetKey(SCAN_ESC)) // Quit
            quit = 1;
        viewRefresh(0);
    } while (!quit);
    /*
     * Clean up
     */
    keyboardUninstallDriver();
    if (kbhit()) getch(); // Clean up any straggling keypresses
    viewExit();
    txt80();
    return 0;
}
