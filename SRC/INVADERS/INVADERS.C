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

#define SHIP_Y  104
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
unsigned char far *ship, far *missile, far *fireball;
int shipWidth, shipHeight;
int missileWidth, missileHeight, sizeofMissile;
int fireballSeq, fireballWidth, fireballHeight, sizeofFireball;

int main(int argc, char **argv)
{
    unsigned long st;
    int quit, oldX, shipX, shipY;
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
    if (!(fireballSeq = spriteLoad("fireball.spr", &fireball, &fireballWidth, &fireballHeight)))
    {
        fprintf(stderr, "Unable to load missile sprite page\n");
        exit(1);
    }
    sizeofFireball = fireballWidth * fireballHeight / 2;
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
    shipX           = 96;
    missileInFlight = 0;
    keyboardInstallDriver();
    viewInit(gr160(BLACK, BLACK), 16, 16, mapWidth, mapHeight, (unsigned char far * far *)tilemap);
    spriteEnable(0, shipX, SHIP_Y, shipWidth, shipHeight, ship);
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
            missileY -= 2;
            if (missileY < 8)
            {
                /*
                 * Missile off-map
                 */
                missileInFlight = 0;
                spriteDisable(1);
            }
            else
            {
                spritePosition(1, missileX, missileY);
            }
        }
        oldX = shipX;
        if (keyboardGetKey(SCAN_KP_4) || keyboardGetKey(SCAN_LEFT_ARROW)) // Turn left
            shipX--;
        if (keyboardGetKey(SCAN_KP_6) || keyboardGetKey(SCAN_RIGHT_ARROW)) // Turn right
            shipX++;
        if (oldX != shipX)
            spritePosition(0, shipX, SHIP_Y);
        if (!missileInFlight && keyboardGetKey(SCAN_SPACE)) // Fire missile
        {
            missileX        = shipX + 2;
            missileY        = SHIP_Y;
            missileInFlight = spriteEnable(1, missileX, missileY, missileWidth, missileHeight, missile);
        }
        if (keyboardGetKey(SCAN_ESC)) // Quit, but don't allow cheating if about to be hit by SAM
            quit = 1;
        viewRefresh(0);
    } while (!quit);
    keyboardUninstallDriver();
    if (kbhit()) getch(); // Clean up any straggling keypresses
    viewExit();
    txt80();
    return 0;
}
