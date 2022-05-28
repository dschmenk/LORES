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

#define ST_RANGE    (16*12)

unsigned long st2xy(int s, int t)
{
    int x, y;

    x = s + t;
    y = (s >> 1) - (t >> 1);
    return ((unsigned long)x & 0xFFFF) | ((unsigned long)y << 16);
}
unsigned long xy2st(int x, int y)
{
    int s, t;

    s = (x + (y << 1)) >> 1;
    t = (x - (y << 1)) >> 1;
    return ((unsigned long)s & 0xFFFF) | ((unsigned long)t << 16);
}
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
 * Isometric tile map test rig
 */
int main(int argc, char **argv)
{
    int x, y, s, t, quit;
    int scrollDir, viewScreenX, viewScreenY, viewWorldX, viewWorldY;
    unsigned long xy, st;

    if ((tileCount = tilesetLoad("isotest.set", (unsigned char far * *)&tileset, sizeof(struct tile))) == 0)
    {
        fprintf(stderr, "Unable to load tile set\n");
        exit(1);
    }
    st = tilemapLoad("isotest.map", (unsigned char far *)tileset, sizeof(struct tile), (unsigned char far * far * *)&tilemap);
    if (!st)
    {
        fprintf(stderr, "Unable to load tile map\n");
        exit(1);
    }
    mapWidth    = st;
    mapHeight   = st >> 16;
    viewScreenX = 0;
    viewScreenY = 0;
    quit        = 0;
    keyboardInstallDriver();
    viewInit(gr160(BLACK, BLACK), viewScreenY, viewScreenX, mapWidth, mapHeight, (unsigned char far * far *)tilemap);
    while (!quit)
    {
        scrollDir = 0;
        if (keyboardGetKey(SCAN_KP_4) || keyboardGetKey(SCAN_LEFT_ARROW)) // Move left
            scrollDir |= SCROLL_RIGHT2 | SCROLL_DOWN;
        if (keyboardGetKey(SCAN_KP_6) || keyboardGetKey(SCAN_RIGHT_ARROW)) // Move right
            scrollDir |= SCROLL_LEFT2 | SCROLL_UP;
        if (keyboardGetKey(SCAN_KP_8) || keyboardGetKey(SCAN_UP_ARROW)) //  Speed up
            scrollDir |= SCROLL_LEFT2 | SCROLL_DOWN;
        if (keyboardGetKey(SCAN_KP_2) || keyboardGetKey(SCAN_DOWN_ARROW)) // Slow down
            scrollDir |= SCROLL_RIGHT2 | SCROLL_UP;
        if (!quit && keyboardGetKey(SCAN_ESC)) // Quit
            quit = 1;
        st = viewRefresh(scrollDir);
        viewScreenX = st;
        viewScreenY = st >> 16;
    }
    keyboardUninstallDriver();
    if (kbhit()) getch(); // Clean up any straggling keypresses
    viewExit();
    txt80();
    return 0;
}
