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
#define SCREEN_WIDTH        160
#define SCREEN_HEIGHT       100
#define ORG_X               16
#define ORG_Y               96
#define SHIP_Y              (ORG_Y+SCREEN_HEIGHT-shipHeight)
#define MAX_INVADERS        5
#define SHIP_SPRITE         0
#define MISSILE_SPRITE      1
#define INVADER_SPRITE_BASE 2
#define INVADER_ALIVE       0
#define INVADER_INC         5
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

int main(int argc, char **argv)
{
    unsigned long st;
    int quit, oldX, shipX, i;
    int missileInFlight, missileX, missileY;

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
        invaderY[invaderCount]     = ORG_Y + 1;
        spriteEnable(invaderCount + INVADER_SPRITE_BASE, invaderX[invaderCount], invaderY[invaderCount], invaderWidth, invaderHeight, invader);
    }
    /*
     * Scroll into view
     */
    for (i = 0; i < ORG_Y; i++)
        viewRefresh(SCROLL_UP);
    do
    {
#ifdef PROFILE
        rasterBorder(WHITE);
#endif
        /*
         * Update missile position
         */
        if (missileInFlight)
        {
            missileY -= 4;
            /*
             * Check missile hit on alien
             */
            for (i = 0; i < MAX_INVADERS; i++)
                if (invaderState[i] == INVADER_ALIVE
                && (missileX >= invaderX[i])
                && (missileX + missileWidth <= invaderX[i] + invaderWidth)
                && (missileY >= invaderY[i])
                && (missileY + missileHeight <= invaderY[i] + invaderHeight))
                {
                    invaderState[i]++;
                    missileY = 0;
                }
            if (missileY < ORG_Y-16)
            {
                /*
                 * Missile off-map
                 */
                missileInFlight = 0;
                spriteDisable(MISSILE_SPRITE);
            }
            else
                spritePosition(MISSILE_SPRITE, missileX, missileY);
        }
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
            spritePosition(invaderMove + INVADER_SPRITE_BASE, invaderX[invaderMove], invaderY[invaderMove]);
            if ((invaderY[invaderMove] + invaderHeight >= SHIP_Y)
             && (invaderX[invaderMove] + invaderWidth >= shipX)
             && (invaderX[invaderMove] <= shipX + shipWidth))
             {
                quit = MAX_INVADERS;
                invaderMove++;
            }
        }
        else
        {
            if (invaderState[invaderMove] < invaderSeq)
                spriteUpdate(invaderMove + INVADER_SPRITE_BASE, invader + invaderState[invaderMove]++ * sizeofInvader);
            else if (invaderState[invaderMove] == invaderSeq)
            {
                invaderState[invaderMove]++;
                spriteDisable(invaderMove + INVADER_SPRITE_BASE);
                quit = (--invaderCount == 0) ? -1 : 0;
            }
        }
        if (!invaderMove--)
            invaderMove = MAX_INVADERS - 1;
        oldX = shipX;
        if (keyboardGetKey(SCAN_KP_4) || keyboardGetKey(SCAN_LEFT_ARROW)) // Turn left
            shipX--;
        if (keyboardGetKey(SCAN_KP_6) || keyboardGetKey(SCAN_RIGHT_ARROW)) // Turn right
            shipX++;
        if (oldX != shipX)
        {
            if (shipX < ORG_X-3 || shipX > ORG_X+156)
                shipX = oldX;
            else
                spritePosition(SHIP_SPRITE, shipX, SHIP_Y);
        }
        if (!missileInFlight && keyboardGetKey(SCAN_SPACE)) // Fire missile
        {
            missileX        = shipX + 2;
            missileY        = SHIP_Y - missileHeight;
            missileInFlight = spriteEnable(MISSILE_SPRITE, missileX, missileY, missileWidth, missileHeight, missile);
        }
        if (keyboardGetKey(SCAN_ESC)) // Quit, but don't allow cheating if about to be hit by SAM
            quit = -1;
        viewRefresh(0);
    } while (!quit);
    /*
     * Scroll out of view
     */
    if (missileInFlight)
        spriteDisable(MISSILE_SPRITE);
    if (quit > 0)
        spriteDisable(SHIP_SPRITE);
    while (quit > 0)
    {
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
        viewRefresh(SCROLL_DOWN);
        quit--;
    }
    for (i = 0; i < ORG_Y; i++)
        viewRefresh(SCROLL_DOWN);
    keyboardUninstallDriver();
    if (kbhit()) getch(); // Clean up any straggling keypresses
    viewExit();
    txt80();
    return 0;
}
