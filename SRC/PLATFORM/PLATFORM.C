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
int heroWidth, heroHeight, sizeofHero;

int main(int argc, char **argv)
{
    unsigned long st;
    signed   char quit, tileAtFlags, tileOnFlags, scrolldir;
    int heroS, heroT, heroPrevS, heroPrevT, heroMapX, heroMapY;
    int heroVelS, heroVelT, heroOfstS, heroOfstT;
    unsigned int heroSprite, heroPrevSprite;
    int mapOrgS, mapOrgT;

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
    tileset[4].tileFlags = TILE_SOLID;
    tileset[5].tileFlags = TILE_SOLID|TILE_LADDER;
    tileset[6].tileFlags = TILE_GROUND;
    st = tilemapLoad("world.map", (unsigned char far *)tileset, sizeof(struct tile), (unsigned char far * far * *)&tilemap);
    if (!st)
    {
        fprintf(stderr, "Unable to load tile map\n");
        exit(1);
    }
    mapWidth        = st;
    mapHeight       = st >> 16;
    mapOrgS         =
    mapOrgT         = 0;
    quit            = 0;
    heroVelS        =
    heroVelT        = 0;
    heroS           = SCREEN_WIDTH/2;
    heroT           = heroHeight - 1;
    heroSprite      = HERO_IDLE;
    keyboardInstallDriver();
    viewInit(gr160(BLUE, BLUE), 0, 0, mapWidth, mapHeight, (unsigned char far * far *)tilemap);
    spriteEnable(HERO_SPRITE, heroS + heroWidth/2, heroT - heroHeight, heroWidth, heroHeight, hero + heroSprite);
    do
    {
#ifdef PROFILE
        rasterBorder(WHITE);
#endif
        if (frameCount & 1)
        {
            heroPrevS      = heroS;
            heroPrevT      = heroT;
            heroMapX       = heroS >> 4;
            heroMapY       = heroT >> 4;
            tileAtFlags    = tilemap[ heroMapY      * mapWidth + heroMapX]->tileFlags;
            tileOnFlags    = tilemap[(heroMapY + 1) * mapWidth + heroMapX]->tileFlags;
            heroPrevSprite = heroSprite;
            /*
             * Hero movement input
             */
            if ((heroT & 0x000F) == 0x0F)
            {
                if (tileOnFlags == TILE_AIR) // Fall
                {
                    heroS     += heroVelS;
                    heroT     += heroVelT >> 1;
                    heroSprite = (heroVelS > 0 ? HERO_JUMP_RIGHT : HERO_JUMP_LEFT) * sizeofHero;
                    if (++heroVelT > 15) heroVelT = 15; // Terminal velocity
                }
                else // Standing on some kind of surface
                {
                    heroVelS =
                    heroVelT = 0;
                    if ((tileAtFlags & TILE_LADDER) && ((keyboardGetKey(SCAN_KP_8) || keyboardGetKey(SCAN_UP_ARROW)))) // Climb up
                    {
                        heroS = (heroS & 0xFFF0) | 0x8;
                        heroT--;
                        heroSprite = (HERO_CLIMB + ((heroT >> 1) & 0x03)) * sizeofHero;
                    }
                    else if ((tileOnFlags & TILE_LADDER) && ((keyboardGetKey(SCAN_KP_2) || keyboardGetKey(SCAN_DOWN_ARROW)))) // Climb down
                    {
                        heroS = (heroS & 0xFFF0) | 0x8;
                        heroT++;
                        heroSprite = (HERO_CLIMB + ((heroT >> 1) & 0x03)) * sizeofHero;
                    }
                    else if (tileOnFlags & TILE_GROUND)
                    {
                        if (keyboardGetKey(SCAN_KP_4) || keyboardGetKey(SCAN_LEFT_ARROW)) // Move left
                        {
                            if ((heroS & 0x0F) > 2 || tilemap[(heroMapY + 1) * mapWidth + heroMapX - 1]->tileFlags != TILE_SOLID)
                            {
                                heroVelS = -2;
                                heroS   -= 2;
                                heroSprite = (HERO_RUN_LEFT + ((heroS >> 3) & 0x03)) * sizeofHero;
                            }
                        }
                        if (keyboardGetKey(SCAN_KP_6) || keyboardGetKey(SCAN_RIGHT_ARROW)) // Move right
                        {
                            if ((heroS & 0x0F) < 14 || tilemap[(heroMapY + 1) * mapWidth + heroMapX + 1]->tileFlags != TILE_SOLID)
                            {
                                heroVelS = 2;
                                heroS   += 2;
                                heroSprite = (HERO_RUN_RIGHT + ((heroS >> 3) & 0x03)) * sizeofHero;
                            }
                        }
                        if (heroS == heroPrevS && heroT == heroPrevT) //Idle breathing
                        {
                            heroSprite = (HERO_IDLE + ((frameCount >> 6) & 0x01)) * sizeofHero;
                        }
                    }
                }
            }
            else // Mid-tile
            {
                if (tileAtFlags & TILE_LADDER) // Climbing
                {
                    if (keyboardGetKey(SCAN_KP_8) || keyboardGetKey(SCAN_UP_ARROW)) // Climb up
                    {
                        heroS = (heroS & 0xFFF0) | 0x8;
                        heroT--;
                        heroSprite = (HERO_CLIMB + ((heroT >> 1) & 0x03)) * sizeofHero;
                    }
                    if (keyboardGetKey(SCAN_KP_2) || keyboardGetKey(SCAN_DOWN_ARROW)) // Climb down
                    {
                        heroS = (heroS & 0xFFF0) | 0x8;
                        heroT++;
                        heroSprite = (HERO_CLIMB + ((heroT >> 1) & 0x03)) * sizeofHero;
                    }
                }
                else if (tileAtFlags & TILE_GROUND) // Fell into ground tile so force to surface
                {
                    heroT      = (heroT & 0xFFF0) - 1;
                    heroSprite = HERO_IDLE;
                    heroVelT   = 0;
                }
                else // Falling
                {
                    heroS     += heroVelS;
                    heroT     += heroVelT >> 1;
                    heroSprite = (heroVelS > 0 ? HERO_JUMP_RIGHT : HERO_JUMP_LEFT) * sizeofHero;
                    if (++heroVelT > 15) heroVelT = 15; // Terminal velocity
                }
            }
            if (heroS != heroPrevS || heroT != heroPrevT)
            {
                st    = spritePosition(HERO_SPRITE, heroS - heroOfstS, heroT - heroOfstT);
                heroS =  st        + heroOfstS;
                heroT = (st >> 16) + heroOfstT;
            }
            if (heroSprite != heroPrevSprite)
                spriteUpdate(HERO_SPRITE, hero + heroSprite);
        }
        /*
         * Attempt to keep hero centered by scrolling map
         */
        scrolldir = 0;
        if (heroS - heroOfstS - SCREEN_WIDTH/2 + 2 < mapOrgS)
            scrolldir = SCROLL_RIGHT2;
        else if (heroS - heroOfstS - SCREEN_WIDTH/2 - 2> mapOrgS)
            scrolldir = SCROLL_LEFT2;
        if (heroT - heroOfstT - SCREEN_HEIGHT/2 < mapOrgT)
            scrolldir |= SCROLL_DOWN;
        else if (heroT - heroOfstT - SCREEN_HEIGHT/2 > mapOrgT)
            scrolldir |= SCROLL_UP;
        st      = viewRefresh(scrolldir);
        mapOrgS = st;
        mapOrgT = st >> 16;
        if (keyboardGetKey(SCAN_ESC)) // Quit
            quit = 1;
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
