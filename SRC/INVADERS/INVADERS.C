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
#define ORG_X                   16
#define ORG_Y                   96
/*
 * Game defines
 */
#define SHIP_Y                  (ORG_Y+SCREEN_HEIGHT-shipHeight)
#define SHIP_SPRITE             0
#define MISSILE_SPRITE          1
#define MISSILE_SPEED           4
#define PEWPEW_COUNT            8
#define MAX_INVADERS            6
#define INVADER_INC             (MAX_INVADERS/2)
#define INVADER_SPRITE_BASE     2
#define INVADER_ALIVE           0
#define QUIT_SUCCESS            -2
#define QUIT_BAIL               -1
#define QUIT_FAIL               (invaderSeq+1)
/*
 * Tile map
 */
int mapWidth, mapHeight;
struct tile
{
    unsigned char tilePix[16*16/2];
} far *tileset, far * far *tilemap;
int tileCount;
/*
 * Sprites
 */
unsigned char far *ship, far *missile, far *invader;
int shipWidth, shipHeight;
int missileWidth, missileHeight, sizeofMissile;
int invaderInc[MAX_INVADERS], invaderX[MAX_INVADERS], invaderY[MAX_INVADERS], invaderState[MAX_INVADERS];
int invaderMove, invaderCount, invaderSeq, invaderWidth, invaderHeight, sizeofInvader;
/*
 * Sound sequences
 */
int alienBuzz[] =
{
    0x8000,
    0x4000
};
int missileWhiz[] =
{
    0x2C00,
    0x2A00,
    0x2800,
    0x2400,
    0x2200,
    0x2100,
    0x2000,
    0x1000,
};

int main(int argc, char **argv)
{
    unsigned long st;
    int quit, oldX, shipX, i, buzzRate, buzzIndex;
    int missileInFlight, missileX, missileY, pewpew;

    if (!spriteLoad("ship.spr", &ship, &shipWidth, &shipHeight))
    {
        fprintf(stderr, "Unable to load ship sprite page\n");
        exit(1);
    }
    if (!spriteLoad("missile.spr", &missile, &missileWidth, &missileHeight))
    {
        fprintf(stderr, "Unable to load missile sprite page\n");
        exit(1);
    }
    if (!(invaderSeq = spriteLoad("invader.spr", &invader, &invaderWidth, &invaderHeight)))
    {
        fprintf(stderr, "Unable to load invader sprite page\n");
        exit(1);
    }
    sizeofInvader = invaderWidth * invaderHeight / 2;
    if ((tileCount = tilesetLoad("earth.set", (unsigned char far * *)&tileset, sizeof(struct tile))) == 0)
    {
        fprintf(stderr, "Unable to load tile set\n");
        exit(1);
    }
    st = tilemapLoad("earth.map", (unsigned char far *)tileset, sizeof(struct tile), (unsigned char far * far * *)&tilemap);
    if (!st)
    {
        fprintf(stderr, "Unable to load tile map\n");
        exit(1);
    }
    mapWidth        = st;
    mapHeight       = st >> 16;
    quit            = 0;
    shipX           = ORG_X + 160/2 - 3;
    missileInFlight = 0;
    invaderMove     = MAX_INVADERS - 1;
    keyboardInstallDriver();
    viewInit(gr160(BLACK, BLACK), ORG_X, 0, mapWidth, mapHeight, (unsigned char far * far *)tilemap);
    spriteEnable(SHIP_SPRITE, shipX, SHIP_Y, shipWidth, shipHeight, ship);
    for (invaderCount = 0; invaderCount < MAX_INVADERS; invaderCount++)
    {
        invaderState[invaderCount] = INVADER_ALIVE;
        invaderInc[invaderCount]   = INVADER_INC;
        invaderX[invaderCount]     = ORG_X + SCREEN_WIDTH/MAX_INVADERS * invaderCount + SCREEN_WIDTH/MAX_INVADERS/4;
        invaderY[invaderCount]     = ORG_Y + 1 + invaderCount * (invaderHeight + 1);
        spriteEnable(invaderCount + INVADER_SPRITE_BASE, invaderX[invaderCount], invaderY[invaderCount], invaderWidth, invaderHeight, invader);
    }
    /*
     * Scroll into view
     */
    for (i = 0; i < ORG_Y; i++)
        viewRefresh(SCROLL_UP);
    pewpew    = 0;
    buzzRate  = invaderCount << 2;
    buzzIndex = 1;
    SET_SOUND(alienBuzz[buzzIndex]);
    START_SOUND;
    do
    {
#ifdef PROFILE
        rasterBorder(WHITE);
#endif
        /*
         * Update aliens
         */
        if (invaderState[invaderMove] == INVADER_ALIVE)
        {
            invaderX[invaderMove] += invaderInc[invaderMove];
            if (invaderX[invaderMove] < ORG_X || invaderX[invaderMove] > ORG_X + SCREEN_WIDTH - invaderWidth)
            {
                invaderInc[invaderMove] = -invaderInc[invaderMove];
                invaderX[invaderMove]   = (invaderX[invaderMove] < ORG_X) ? ORG_X : ORG_X + SCREEN_WIDTH - invaderWidth;
                invaderY[invaderMove]  += invaderHeight + 1;
            }
            if ((invaderY[invaderMove] + invaderHeight >= SHIP_Y)
             && (invaderX[invaderMove] + invaderWidth > shipX)
             && (invaderX[invaderMove] < shipX + shipWidth))
            {
                /*
                 * Alien hit ship
                 */
                invaderState[invaderMove]++;
                spriteDisable(SHIP_SPRITE);
                invaderX[invaderMove] = shipX;
                invaderY[invaderMove] = SHIP_Y;
                quit                  = QUIT_FAIL;
            }
            spritePosition(invaderMove + INVADER_SPRITE_BASE, invaderX[invaderMove], invaderY[invaderMove]);
        }
        else
        {
            if (invaderState[invaderMove] < invaderSeq)
                spriteUpdate(invaderMove + INVADER_SPRITE_BASE, invader + invaderState[invaderMove]++ * sizeofInvader);
            else if (invaderState[invaderMove] == invaderSeq)
            {
                invaderState[invaderMove]++;
                spriteDisable(invaderMove + INVADER_SPRITE_BASE);
                quit = (--invaderCount == 0) ? QUIT_SUCCESS : 0;
            }
        }
        if (!quit)
            do
            {
                /*
                 * Find next active alien
                 */
                invaderMove = !invaderMove ? invaderMove = MAX_INVADERS - 1 : invaderMove - 1;
            } while (invaderState[invaderMove] > invaderSeq);
        if (missileInFlight > 0)
        {
            /*
             * Update missile position
             */
            missileY -= MISSILE_SPEED;
            if (missileY < ORG_Y-16)
            {
                /*
                 * Missile off-map
                 */
                missileInFlight = 0;
                spriteDisable(MISSILE_SPRITE);
            }
            else
            {
                spritePosition(MISSILE_SPRITE, missileX, missileY);
                /*
                 * Check missile hit on alien
                 */
                for (i = 0; i < MAX_INVADERS; i++)
                    if ((invaderState[i] == INVADER_ALIVE)
                     && (missileY + missileHeight > invaderY[i])
                     && (missileY < invaderY[i] + invaderHeight)
                     && (missileX + missileWidth > invaderX[i])
                     && (missileX < invaderX[i] + invaderWidth))
                    {
                        invaderState[i]++;
                        missileInFlight = (ORG_Y-16 - missileY) / MISSILE_SPEED;
                        spriteDisable(MISSILE_SPRITE);
                    }
            }
        }
        else if (missileInFlight < 0)
            /*
             * Missile reload time
             */
            missileInFlight++;
        else if (keyboardGetKey(SCAN_SPACE))
        {
            /*
             * Check for missile fired
             */
            pewpew          = PEWPEW_COUNT;
            missileX        = shipX + 2;
            missileY        = SHIP_Y - missileHeight;
            missileInFlight = spriteEnable(MISSILE_SPRITE, missileX, missileY, missileWidth, missileHeight, missile);
        }
        /*
         * Ship movement input
         */
        oldX = shipX;
        if (keyboardGetKey(SCAN_KP_4) || keyboardGetKey(SCAN_LEFT_ARROW)) // Move left
            shipX--;
        if (keyboardGetKey(SCAN_KP_6) || keyboardGetKey(SCAN_RIGHT_ARROW)) // Move right
            shipX++;
        if (oldX != shipX)
        {
            if (shipX < ORG_X-3 || shipX > ORG_X+156)
                shipX = oldX;
            else
                spritePosition(SHIP_SPRITE, shipX, SHIP_Y);
        }
        if (!quit && keyboardGetKey(SCAN_ESC)) // Quit
            quit = QUIT_BAIL;
        /*
         * Update sounds
         */
        if (pewpew)
        {
            if (buzzRate == 0)
                buzzIndex ^= 1;
            if (--pewpew)
                SET_SOUND(missileWhiz[pewpew]);
            else
                SET_SOUND(alienBuzz[buzzIndex]);
        }
        else
        {
            if (buzzRate == 0)
            {
                buzzIndex ^= 1;
                SET_SOUND(alienBuzz[buzzIndex]);
            }
        }
        buzzRate = buzzRate == 0 ? invaderCount << 2: buzzRate - 1;
        viewRefresh(0);
    } while (!quit);
    /*
     * Clean up
     */
    STOP_SOUND;
    if (missileInFlight)
        spriteDisable(MISSILE_SPRITE);
    /*
     * Success/Fail banner
     */
    if (quit == QUIT_SUCCESS)
        text(32, 0, DARKGREEN, "Earth Saved!");
    else
    {
        while (quit > 0)
        {
            /*
             * Ship destroyed by alien so animate final explosion
             */
            if (invaderState[invaderMove] < invaderSeq)
                spriteUpdate(invaderMove + INVADER_SPRITE_BASE, invader + invaderState[invaderMove]++ * sizeofInvader);
            else if (invaderState[invaderMove] == invaderSeq)
            {
                invaderState[invaderMove]++;
                spriteDisable(invaderMove + INVADER_SPRITE_BASE);
            }
            frameCount = 0;
            while (frameCount < MAX_INVADERS)
                statusRasterTimer();
            viewRefresh(0);
            quit--;
        }
        text(24, 0, DARKRED, "Earth Invaded!");
    }
    /*
     * Hold screen for 2 seconds
     */
    frameCount = 0;
    while (frameCount < 120)
        statusRasterTimer();
    /*
     * Scroll out of view
     */
    for (i = 0; i < ORG_Y; i++)
        viewRefresh(SCROLL_DOWN);
    keyboardUninstallDriver();
    if (kbhit()) getch(); // Clean up any straggling keypresses
    viewExit();
    txt80();
    return 0;
}
