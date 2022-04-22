#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <conio.h>
#include "lores.h"
#include "tiler.h"
#include "mapio.h"
#ifndef USE_GETCH
#include "keyboard.h"
#endif
#define ESCAPE          0x001B
#define SPACEBAR        0x0020
#define LEFT_ARROW      0x4B00
#define RIGHT_ARROW     0x4D00
#define UP_ARROW        0x4800
#define DOWN_ARROW      0x5000
/*
 * PPI stands for Programmable Peripheral Interface (which is the Intel 8255A chip)
 * The PPI ports are only for IBM PC and XT, however port A is mapped to the same
 * I/O address as the Keyboard Controller's (Intel 8042 chip) output buffer for compatibility.
 */
#define PPI_PORT_A 0x60
#define PPI_PORT_B 0x61
#define PPI_PORT_C 0x62
#define PPI_COMMAND_REGISTER 0x63
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
 * Tile indeces for enemy
 */
#define TANK_UP                 52
#define TANK_DOWN               54
#define TANK_LEFT               56
#define TANK_RIGHT              58
#define SAM_LAUNCHER            60
/*
 * Drone speeds
 */
#define MAX_SPEED               2
#define MIN_SPEED               5
#define turnAngle               4
/*
 * Sound sequences
 */
int droneBuzz[] = {
    0xFFFF,
    0x7000,
    0x7800,
    0x8000,
    0x8800,
    0x9000,
    0x9800,
    0xA000,
    0xA800,
};
int explodeNoise[] = {
    0xFD00,
    0xFD80,
    0xFE00,
    0xFE00,
    0xFF80,
    0xFFFF,
};
int missileWhiz[] = {
    0x4000,
    0x4C00,
    0x4100,
    0x4400,
};
int samAlert[] = {
    0x1000,
    0x1800,
    0x1000,
    0x0800,
};
/*
 * Missile flight time
 */
#define MISSILE_FLIGHT_TIME     35
/*
 * SAM flight time
 */
#define SAM_FLIGHT_TIME         100
/*
 * SAM launch distance
 */
#define SAM_RANGE               4
/*
 * 16.16 fixed point sine/cosine
 */
long sinFix[] = {
    0x00000000,
    0x000061f7,
    0x0000b504,
    0x0000ec83,
};
long cosFix[] = {
    0x00010000,
    0x0000ec83,
    0x0000b504,
    0x000061f7,
    0x00000000,
    0xffff9e09,
    0xffff4afc,
    0xffff137d,
    0xffff0000,
    0xffff137d,
    0xffff4afc,
    0xffff9e09,
    0x00000000,
    0x000061f7,
    0x0000b504,
    0x0000ec83,
};
#define XMAJOR  1
#define YMAJOR  0
struct step
{
    long fixInc;
    int  xStep;
    int  yStep;
    char axis;
    char err;
    int  dx2;
    int  dy2;
} dirSteps[16] =
{
    {0x00010000, 2, 2, XMAJOR, -8, 16, 0}, //[0,0],[1,0],[2,0],[3,0],[4,0],[5,0],[6,0],[7,0],[8,0],
    {0x0000EC83, 2, 2, XMAJOR, -1, 14, 6}, //[0,0],[1,0],[2,1],[3,1],[4,2],[5,2],[6,3],[7,3],
    {0x0000B504, 2, 2, XMAJOR, 5, 10, 10}, //[0,0],[1,1],[2,2],[3,3],[4,4],[5,5],
    {0x0000EC83, 2, 2, YMAJOR, -1, 6, 14}, //[0,0],[0,1],[1,2],[1,3],[2,4],[2,5],[3,6],[3,7],
    {0x00010000, 2, 2, YMAJOR, -8, 0, 16}, //[0,0],[0,1],[0,2],[0,3],[0,4],[0,5],[0,6],[0,7],[0,8],
    {0x0000EC83, -2, 2, YMAJOR, -1, 6, 14}, //[0,0],[0,1],[-1,2],[-1,3],[-2,4],[-2,5],[-3,6],[-3,7],
    {0x0000B504, -2, 2, XMAJOR, 5, 10, 10}, //[0,0],[-1,1],[-2,2],[-3,3],[-4,4],[-5,5],
    {0x0000EC83, -2, 2, XMAJOR, -1, 14, 6}, //[0,0],[-1,0],[-2,1],[-3,1],[-4,2],[-5,2],[-6,3],[-7,3],
    {0x00010000, -2, 2, XMAJOR, -8, 16, 0}, //[0,0],[-1,0],[-2,0],[-3,0],[-4,0],[-5,0],[-6,0],[-7,0],[-8,0],
    {0x0000EC83, -2, -2, XMAJOR, -1, 14, 6}, //[0,0],[-1,0],[-2,-1],[-3,-1],[-4,-2],[-5,-2],[-6,-3],[-7,-3],
    {0x0000B504, -2, -2, XMAJOR, 5, 10, 10}, //[0,0],[-1,-1],[-2,-2],[-3,-3],[-4,-4],[-5,-5],
    {0x0000EC83, -2, -2, YMAJOR, -1, 6, 14}, //[0,0],[0,-1],[-1,-2],[-1,-3],[-2,-4],[-2,-5],[-3,-6],[-3,-7],
    {0x00010000, 2, -2, YMAJOR, -8, 0, 16}, //[0,0],[0,-1],[0,-2],[0,-3],[0,-4],[0,-5],[0,-6],[0,-7],[0,-8],
    {0x0000EC83, 2, -2, YMAJOR, -1, 6, 14}, //[0,0],[0,-1],[1,-2],[1,-3],[2,-4],[2,-5],[3,-6],[3,-7],
    {0x0000B504, 2, -2, XMAJOR, 5, 10, 10}, //[0,0],[1,-1],[2,-2],[3,-3],[4,-4],[5,-5],
    {0x0000EC83, 2, -2, XMAJOR, -1, 14, 6}, //[0,0],[1,0],[2,-1],[3,-1],[4,-2],[5,-2],[6,-3],[7,-3],
};
/*
 * Tile map
 */
int mapWidth, mapHeight;
struct tile
{
    unsigned char tilePix[16*16/2];
    int           isTarget;
    int           index;
} far *tileset, far * far *tilemap;
int tileCount;
/*
 * Sprites
 */
unsigned char far *drone, far *missile, far *sam, far *fireball;
int droneWidth, droneHeight, sizeofDrone;
int missileWidth, missileHeight, sizeofMissile;
int samWidth, samHeight, sizeofSAM;
int fireballSeq, fireballWidth, fireballHeight, sizeofFireball;
int numDrones, numTanks, numSAMs, liveTanks, liveSAMs;
/*
 * Display intro screen
 */
void intro(char *txtfile)
{
    FILE *fp;
    char txt[128];

    txt80();
    fp = fopen(txtfile, "r");
    if (fp)
    {
        while (fgets(txt, 128, fp))
            printf("%s", txt);
        fclose(fp);
    }
    else
        printf("Unable to open %s\n", txtfile);
}
/*
 * Convert cartesian coordiantes to angle
 */
#define DIFF_POW    2
unsigned char xy2angle(int x, int y)
{
    int absX, absY;

    absX = x >= 0 ? x : -x;
    absY = y >= 0 ? y : -y;
    if (x >= 0 && y >= 0)
    {
        if (absX > (absY << DIFF_POW))
            return 8;
        else if (absY > (absX << DIFF_POW))
            return 12;
        else return 10;
    }
    else if (x < 0 && y >= 0)
    {
        if (absX > (absY << DIFF_POW))
            return 0;
        else if (absY > (absX << DIFF_POW))
            return 12;
        else return 14;
    }
    else if (x < 0 && y < 0)
    {
        if (absX > (absY << DIFF_POW))
            return 0;
        else if (absY > (absX << DIFF_POW))
            return 4;
        else return 2;
    }
    else if (x >= 0 && y < 0)
    {
        if (absX > (absY << DIFF_POW))
            return 8;
        else if (absY > (absX << DIFF_POW))
            return 4;
        else return 6;
    }
    return 0;
}
/*
 * Demo tiling and scrolling screen
 */
void repelz(void)
{
    int maxS, maxT, viewS, viewT;
    long droneFix, droneInc;
    struct step *droneStep;
    int droneS, droneT, droneMapX, droneMapY, dronePrevMapX, dronePrevMapY, droneErr;
    int droneSpeed, throttle;
    long missileFixS, missileFixT, missileIncS, missileIncT;
    int missileS, missileT, missileInFlight;
    int missileMapX, missileMapY, missilePrevMapX, missilePrevMapY;
    long samFixS, samFixT, samIncS, samIncT;
    int samS, samT, samPrevS, samPrevT, samInFlight;
    unsigned char droneAngle, droneDir, samDir, ending, explosion, quit;
    int diffS, diffT, viewOffsetS, viewOffsetT;
    unsigned long st;
    int scrolldir, i;

    maxS            = mapWidth << 4;
    maxT            = mapHeight << 4;
    viewOffsetS     = 80 - droneWidth / 2;
    viewOffsetT     = 50 - droneHeight / 2;
    droneS          = (maxS >> 1) & 0xFFFE;
    droneT          = (maxT >> 1) & 0xFFFE;
    dronePrevMapX   = 0;
    dronePrevMapY   = 0;
    droneAngle      = 0x08;
    droneDir        = 0;
    droneSpeed      = MIN_SPEED;
    droneStep       = &dirSteps[droneDir];
    droneInc        = droneStep->fixInc / droneSpeed;
    droneErr        = droneStep->err;
    droneFix        = 0L;
    viewS           = (droneS - viewOffsetS) & 0xFFFE;
    viewT           = (droneT - viewOffsetT) & 0xFFFE;
    throttle        = droneSpeed << 3;
    samInFlight     = 0;
    missileInFlight = 0;
    explosion       = 0;
    ending          = 0;
    quit            = 0;
    keyboardInstallDriver();
    viewInit(gr160(BLACK, BLACK), viewS, viewT, mapWidth, mapHeight, (unsigned char far * far *)tilemap);
    spriteEnable(0, droneS, droneT, droneWidth, droneHeight, drone + droneDir * sizeofDrone);
    SET_SOUND(droneBuzz[droneSpeed]);
    START_SOUND;
    do
    {
#ifdef PROFILE
        //rasterBorder(WHITE);
#endif
        scrolldir = 0;
        /*
         * Update drone position
         */
        droneFix += droneInc;
        if (droneFix >> 16) // Check for integer quantity > 0
        {
            droneFix &= 0x0000FFFFL; // Zero out integer quantity
            if (droneStep->axis == XMAJOR)
            {
                if (droneErr >= 0)
                {
                    droneErr -= droneStep->dx2;
                    droneT   += droneStep->yStep;
                }
                droneErr += droneStep->dy2;
                droneS   += droneStep->xStep;
            }
            else
            {
                if (droneErr >= 0)
                {
                    droneErr -= droneStep->dy2;
                    droneS  += droneStep->xStep;
                }
                droneErr += droneStep->dx2;
                droneT   += droneStep->yStep;
            }
            droneMapX  = droneS >> 4;
            droneMapY  = droneT >> 4;
            /*
             * Attempt to keep drone centered by scrolling map
             */
            if (droneS < viewS + viewOffsetS - 1)
                scrolldir = SCROLL_RIGHT2;
            else if (droneS > viewS + viewOffsetS + 1)
                scrolldir = SCROLL_LEFT2;
            if (droneT < viewT + viewOffsetT - 1)
                scrolldir |= SCROLL_DOWN2;
            else if (droneT > viewT + viewOffsetT + 1)
                scrolldir |= SCROLL_UP2;
            if (droneS < 0 || droneS > maxS - droneWidth || droneT < 0 || droneT > maxT - droneHeight)
            {
                /*
                 * Drone explodes at map edge
                 */
                if (droneS < 0)                       droneS = 0;
                else if (droneS > maxS - droneWidth)  droneS = maxS - droneWidth;
                if (droneT < 0)                       droneT = 0;
                else if (droneT > maxT - droneHeight) droneT = maxT - droneHeight;
                droneInc = 0;
                ending   = 1;
                spriteDisable(0);
                spriteEnable(4, droneS + (droneWidth - fireballWidth)/2, droneT + (droneHeight - fireballHeight)/2, fireballWidth, fireballHeight, fireball);
            }
            spritePosition(0, droneS, droneT);
        }
        /*
         * Update missile position
         */
        if (missileInFlight)
        {
            if (--missileInFlight)
            {
                missileFixS += missileIncS;
                missileS     = missileFixS >> 16;
                missileFixT += missileIncT;
                missileT     = missileFixT >> 16;
                if (missileS < 0 || missileS > (maxS - missileWidth) || missileT < 0 || missileT > (maxT - missileHeight))
                {
                    /*
                     * Missile off-map
                     */
                    missileInFlight = 0;
                    spriteDisable(1);
                }
                else
                {
                    missileMapX = missileS >> 4;
                    missileMapY = missileT >> 4;
                    if (missileMapX != missilePrevMapX || missileMapY != missilePrevMapY)
                    {
                        /*
                         * Check for target hit
                         */
                        if (tilemap[missileMapY * mapWidth + missileMapX]->isTarget)
                        {
                            if (tilemap[missileMapY * mapWidth + missileMapX]->index == SAM_LAUNCHER)
                                liveSAMs--;
                            else
                                liveTanks--;
                            /*
                             * Create explosion
                             */
                            explosion       = 1;
                            missileInFlight = 0;
                            spriteDisable(1);
                            spriteEnable(3, (missileMapX << 4) + 8 - fireballWidth/2, (missileMapY << 4) + 8 - fireballHeight/2, fireballWidth, fireballHeight, fireball);
                            tileUpdate(missileMapX, missileMapY, (unsigned char far *)&tileset[tilemap[missileMapY * mapWidth + missileMapX]->index + 1]);
                        }
                        missilePrevMapX = missileMapX;
                        missilePrevMapY = missileMapY;
                    }
                    spritePosition(1, missileS, missileT);
                }
            }
            else
                spriteDisable(1);
        }
        /*
         * Update SAM position
         */
        if (samInFlight)
        {
            if (--samInFlight)
            {
                samFixS += samIncS;
                samS     = samFixS >> 16;
                samFixT += samIncT;
                samT     = samFixT >> 16;
                if (samS < 0 || samS > (maxS - samWidth) || samT < 0 || samT > (maxT - samHeight))
                {
                    /*
                     * Missile off-map
                     */
                    samInFlight = 0;
                    spriteDisable(2);
                }
                else
                {
                    if (samS != samPrevS || samT != samPrevT)
                    {
                        /*
                         * Check for target hit
                         */
                        if (samT >= droneT && (samT + samHeight) <= (droneT + droneHeight) && samS >= droneS && (samS + samWidth) <= (droneS + droneWidth))
                        {
                            /*
                             * Create explosion
                             */
                            ending      = 1;
                            samInFlight = 0;
                            spriteDisable(0);
                            spriteDisable(2);
                            spriteEnable(4, samS + (samWidth - fireballWidth)/2, samT + (samHeight - fireballHeight)/2, fireballWidth, fireballHeight, fireball);
                        }
                        samPrevS = samS;
                        samPrevT = samT;
                    }
                    spritePosition(2, samS, samT);
                }
            }
            else
                spriteDisable(2);
        }
        /*
         * Mini-scheduler based on video framerate
         */
        switch (frameCount & 0x03)
        {
            case 0: // Deal with user input
                if (keyboardGetKey(SCAN_KP_8) || keyboardGetKey(SCAN_UP_ARROW)) //  Speed up
                {
                    if (droneSpeed > MAX_SPEED)
                    {
                        if ((--throttle >> 3) != droneSpeed)
                        {
                            droneSpeed = throttle >> 3;
                            droneInc   = droneStep->fixInc / droneSpeed;
                        }
                    }
                }
                if (keyboardGetKey(SCAN_KP_2) || keyboardGetKey(SCAN_DOWN_ARROW)) // Slow down
                {
                    if (droneSpeed < MIN_SPEED)
                    {
                        if ((++throttle >> 3) != droneSpeed)
                        {
                            droneSpeed = throttle >> 3;
                            droneInc   = droneStep->fixInc / droneSpeed;
                        }
                    }
                }
                if (keyboardGetKey(SCAN_KP_4) || keyboardGetKey(SCAN_LEFT_ARROW)) // Turn left
                    droneAngle -= turnAngle;
                if (keyboardGetKey(SCAN_KP_6) || keyboardGetKey(SCAN_RIGHT_ARROW)) // Turn right
                    droneAngle += turnAngle;
                if (((droneAngle >> 4) & 0x0F) != droneDir)
                {
                    droneDir  = (droneAngle >> 4) & 0x0F;
                    droneStep = &dirSteps[droneDir];
                    droneInc  = droneStep->fixInc / droneSpeed;
                    droneErr  = droneStep->err;
                    spriteUpdate(0, drone + droneDir * sizeofDrone);
                }
                if (keyboardGetKey(SCAN_SPACE)) // Fire missile
                {
                    if (!missileInFlight)
                    {
                        missileIncS = (cosFix[droneDir] << 1) + cosFix[droneDir] / droneSpeed;
                        missileFixS = ((long)droneS << 16) + missileIncS + ((long)(droneWidth - missileWidth) << 15);
                        missileIncT = (sinFix[droneDir] << 1) + sinFix[droneDir] / droneSpeed;
                        missileFixT = ((long)droneT << 16) + missileIncT + ((long)(droneHeight - missileHeight) << 15);
                        if (spriteEnable(1, missileFixS >> 16, missileFixT >> 16, missileWidth, missileHeight, missile + droneDir * sizeofMissile))
                        {
                            missilePrevMapX = 0;
                            missilePrevMapY = 0;
                            missileInFlight = MISSILE_FLIGHT_TIME;
                        }
                    }
                }
                if (!samInFlight && keyboardGetKey(SCAN_ESC)) // Quit, but don't allow cheating if about to be hit by SAM
                    quit = 1;
                break;
            case 1: // Sound sequencing
                if (samInFlight)
                    SET_SOUND(samAlert[samInFlight & 0x03]);
                else if (explosion)
                    SET_SOUND(explodeNoise[explosion - 1]);
                else if (missileInFlight)
                    SET_SOUND(missileWhiz[missileInFlight & 0x03]);
                else
                    SET_SOUND(droneBuzz[droneSpeed] + (frameCount & 0x0010));
                break;
            case 2: // Explosion animation
                if (explosion)
                {
                    if (explosion >= fireballSeq)
                    {
                        explosion = 0;
                        spriteDisable(3);
                    }
                    else
                        spriteUpdate(3, fireball + explosion++ * sizeofFireball);
                }
                if (ending)
                {
                    if (ending >= fireballSeq)
                    {
                        quit = 1;
                        spriteDisable(4);
                    }
                    else
                        spriteUpdate(4, fireball + ending++ * sizeofFireball);
                }
                break;
            case 3: // Enemy AI
                if (samInFlight)
                {
                    /*
                     * Steer SAM into drone
                     */
                    i = samDir;
                    samDir = xy2angle(samS - droneS, samT - droneT);
                    if (i != samDir)
                    {
                        samIncS = cosFix[samDir];
                        samIncT = sinFix[samDir];
                        samIncS -= samIncS >> 2; // Slow SAM down a little
                        samIncT -= samIncT >> 2;
                        spriteUpdate(2, sam + samDir * sizeofSAM);
                    }
                }
                else if ((droneMapX != dronePrevMapX) || (droneMapY != dronePrevMapY))
                {
                    /*
                     * Check if any SAM launcher in-range of drone
                     */
                    if ((droneMapX < dronePrevMapX) && (droneMapX - SAM_RANGE >= 0))
                    {
                        for (i = droneMapY - SAM_RANGE; i < droneMapY + SAM_RANGE; i++)
                        {
                            if (i >= 0 && i < mapHeight && tilemap[i * mapWidth + droneMapX - SAM_RANGE]->index == SAM_LAUNCHER)
                            {
                                samFixS     = ((long)(droneMapX - SAM_RANGE) << 20) + ((long)(16 - samWidth) << 15);
                                samFixT     = ((long)i << 20) + ((long)(16 - samHeight) << 15);
                                samInFlight = SAM_FLIGHT_TIME;
                                break;
                            }
                        }
                    }
                    else if ((droneMapX > dronePrevMapX) && (droneMapX + SAM_RANGE < mapWidth))
                    {
                        for (i = droneMapY - SAM_RANGE; i < droneMapY + SAM_RANGE; i++)
                        {
                            if (i >= 0 && i < mapHeight && tilemap[i * mapWidth + droneMapX + SAM_RANGE]->index == SAM_LAUNCHER)
                            {
                                samFixS     = ((long)(droneMapX + SAM_RANGE) << 20) + ((long)(16 - samWidth) << 15);
                                samFixT     = ((long)i << 20) + ((long)(16 - samHeight) << 15);
                                samInFlight = SAM_FLIGHT_TIME;
                                break;
                            }
                        }
                    }
                    if (!samInFlight)
                    {
                        if ((droneMapY < dronePrevMapY) && (droneMapY - SAM_RANGE >= 0))
                        {
                            for (i = droneMapX - SAM_RANGE; i < droneMapX + SAM_RANGE; i++)
                            {
                                if (i >= 0 && i < mapWidth && tilemap[(droneMapY - SAM_RANGE) * mapWidth + i]->index == SAM_LAUNCHER)
                                {
                                    samFixS     = ((long)i << 20) + ((long)(16 - samWidth) << 15);
                                    samFixT     = ((long)(droneMapY - SAM_RANGE) << 20) + ((long)(16 - samHeight) << 15);
                                    samInFlight = SAM_FLIGHT_TIME;
                                    break;
                                }
                            }
                        }
                        else if ((droneMapY > dronePrevMapY) && (droneMapY + SAM_RANGE < mapHeight))
                        {
                            for (i = droneMapX - SAM_RANGE; i < droneMapX + SAM_RANGE; i++)
                            {
                                if (i >= 0 && i < mapWidth && tilemap[(droneMapY + SAM_RANGE) * mapWidth + i]->index == SAM_LAUNCHER)
                                {
                                    samFixS     = ((long)i << 20) + ((long)(16 - samWidth) << 15);
                                    samFixT     = ((long)(droneMapY + SAM_RANGE) << 20) + ((long)(16 - samHeight) << 15);
                                    samInFlight = SAM_FLIGHT_TIME;
                                    break;
                                }
                            }
                        }
                    }
                    if (samInFlight)
                    {
                        /*
                         * Launch SAM at drone
                         */
                        samDir  = xy2angle((int)(samFixS >> 16) - droneS, (int)(samFixT >> 16) - droneT);
                        samIncS = cosFix[samDir];
                        samIncT = sinFix[samDir];
                        samIncS -= samIncS >> 2; // Slow SAM down a little
                        samIncT -= samIncT >> 2;
                        if (spriteEnable(2, samFixS >> 16, samFixT >> 16, samWidth, samHeight, sam + samDir * sizeofSAM))
                        {
                            samPrevS = 0;
                            samPrevT = 0;
                        }
                        else
                            samInFlight = 0;
                    }
                }
                dronePrevMapX = droneMapX;
                dronePrevMapY = droneMapY;
                break;
        }
        st    = viewRefresh(scrolldir);
        viewS = st;
        viewT = st >> 16;
    } while (!quit);
    STOP_SOUND;
    if (ending)
    {
        text(16, 60, DARKRED, "Drone Destroyed!");
        frameCount = 0;
        while (frameCount < 120)
            statusRasterTimer();
        numDrones++;
    }
    keyboardUninstallDriver();
    if (kbhit()) getch(); // Clean up any straggling keypresses
    viewExit();
    txt80();
    if (numDrones == 1)
        printf("\n\t1 drone destroyed\n");
    else
        printf("\n\t%d drones destroyed\n", numDrones);
    printf("\n\t%d of %d tanks destroyed\n",       numTanks - liveTanks, numTanks);
    printf("\t%d of %d SAM launchers destroyed\n", numSAMs  - liveSAMs,  numSAMs);
}
int main(int argc, char **argv)
{
    unsigned long st;
    int i, j;

    intro("intro.txt");
    if (!spriteLoad("drone.spr", &drone, &droneWidth, &droneHeight))
    {
        fprintf(stderr, "Unable to load drone sprite page\n");
        exit(1);
    }
    sizeofDrone = droneWidth * droneHeight / 2;
    if (!spriteLoad("missile.spr", &missile, &missileWidth, &missileHeight))
    {
        fprintf(stderr, "Unable to load missile sprite page\n");
        exit(1);
    }
    sizeofMissile = missileWidth * missileHeight / 2;
    if (!spriteLoad("sam.spr", &sam, &samWidth, &samHeight))
    {
        fprintf(stderr, "Unable to load SAM sprite page\n");
        exit(1);
    }
    sizeofSAM = samWidth * samHeight / 2;
    if (!(fireballSeq = spriteLoad("fireball.spr", &fireball, &fireballWidth, &fireballHeight)))
    {
        fprintf(stderr, "Unable to load missile sprite page\n");
        exit(1);
    }
    sizeofFireball = fireballWidth * fireballHeight / 2;
    if ((tileCount = tilesetLoad("warzone.set", (unsigned char far * *)&tileset, sizeof(struct tile))) == 0)
    {
        fprintf(stderr, "Unable to load tile set\n");
        exit(1);
    }
    /*
     * Tag target tiles
     */
    for (i = 0; i < tileCount; i++)
    {
        tileset[i].isTarget = 0;
        tileset[i].index    = i;
    }
    tileset[TANK_UP].isTarget      = 1;
    tileset[TANK_DOWN].isTarget    = 1;
    tileset[TANK_LEFT].isTarget    = 1;
    tileset[TANK_RIGHT].isTarget   = 1;
    tileset[SAM_LAUNCHER].isTarget = 1;
    st = tilemapLoad("warzone.map", (unsigned char far *)tileset, sizeof(struct tile), (unsigned char far * far * *)&tilemap);
    if (!st)
    {
        viewExit();
        txt80();
        fprintf(stderr, "Unable to load tile map\n");
        exit(1);
    }
    mapWidth  = st;
    mapHeight = st >> 16;
    numTanks  = 0;
    numSAMs   = 0;
    numDrones = 0;
    for (j = 0; j < mapHeight; j++)
        for (i = 0; i <mapWidth; i++)
        {
            switch (tilemap[j * mapWidth + i]->index)
            {
                case TANK_UP:
                case TANK_DOWN:
                case TANK_LEFT:
                case TANK_RIGHT:
                    numTanks++;
                    break;
                case SAM_LAUNCHER:
                    numSAMs++;
                    break;
            }
        }
    liveTanks = numTanks;
    liveSAMs  = numSAMs;
    puts("Press a key to continue...");
    getch();
    do
    {
        repelz();
        printf("\n\tContinue playing? (Y/N)"); fflush(stdout);
        i = getch();
    } while (toupper(i) == 'Y');
    return 0;
}
