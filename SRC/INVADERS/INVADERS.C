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
int invaderCount, invaderSeq, invaderWidth, invaderHeight, sizeofInvader;

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
    keyboardInstallDriver();
    viewInit(gr160(BLACK, BLACK), ORG_X, 0, mapWidth, mapHeight, (unsigned char far * far *)tilemap);
    spriteEnable(SHIP_SPRITE, shipX, SHIP_Y, shipWidth, shipHeight, ship);
    for (invaderCount = 0; invaderCount < MAX_INVADERS; invaderCount++)
    {
        invaderState[invaderCount] = INVADER_ALIVE;
        invaderInc[invaderCount]   = 1;
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
        for (i = 0; i < MAX_INVADERS; i++)
        {
            if (invaderState[i] == INVADER_ALIVE)
            {
                invaderX[i] += invaderInc[i];
                if (invaderX[i] < ORG_X || invaderX[i] > ORG_X + SCREEN_WIDTH - invaderWidth)
                {
                    invaderInc[i] = -invaderInc[i];
                    invaderX[i]  += invaderInc[i];
                    invaderY[i]  += invaderHeight;
                    if (invaderY[i] >= ORG_Y + SCREEN_HEIGHT)
                        quit = 1;
                }
                spritePosition(i + INVADER_SPRITE_BASE, invaderX[i], invaderY[i]);
            }
            else
            {
                if (invaderState[i] < invaderSeq)
                    spriteUpdate(i + INVADER_SPRITE_BASE, invader + invaderState[i]++ * sizeofInvader);
                else if (invaderState[i] == invaderSeq)
                {
                    invaderState[i]++;
                    spriteDisable(i + INVADER_SPRITE_BASE);
                    quit = (--invaderCount == 0);
                }
            }
        }
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
            quit = 1;
        viewRefresh(0);
    } while (!quit);
    /*
     * Scroll out of view
     */
    if (missileInFlight)
        spriteDisable(MISSILE_SPRITE);
    for (i = 0; i < ORG_Y; i++)
        viewRefresh(SCROLL_DOWN);
    keyboardUninstallDriver();
    if (kbhit()) getch(); // Clean up any straggling keypresses
    viewExit();
    txt80();
    return 0;
}
