#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include "lores.h"
#include "tiler.h"
#include "mapio.h"
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
#define MAX_SPEED               1
#define MIN_SPEED               3
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
#define MISSILE_FLIGHT_TIME     30
/*
 * SAM flight time
 */
#define SAM_FLIGHT_TIME         60
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
 * Extended keyboard input
 */
unsigned short getkb(void)
{
    unsigned short extch;

    if (!kbhit())
        return 0;
    extch = getch();
    if (!extch)
        extch = getch() << 8;
    return extch;
}
/*
 * Convert cartesian coordiantes to angle
 */
unsigned char xy2angle(int x, int y)
{
    int absX, absY;

    absX = x >= 0 ? x : -x;
    absY = y >= 0 ? y : -y;
    if (x >= 0 && y >= 0)
    {
        if (absX > absY << 2)
            return 8;
        else if (absY > absX << 2)
            return 12;
        else return 10;
    }
    else if (x < 0 && y >= 0)
    {
        if (absX > absY << 2)
            return 0;
        else if (absY > absX << 2)
            return 12;
        else return 14;
    }
    else if (x < 0 && y < 0)
    {
        if (absX > absY << 2)
            return 0;
        else if (absY > absX << 2)
            return 4;
        else return 2;
    }
    else if (x >= 0 && y < 0)
    {
        if (absX > absY << 2)
            return 8;
        else if (absY > absX << 2)
            return 4;
        else return 6;
    }
}
/*
 * Demo tiling and scrolling screen
 */
int main(int argc, char **argv)
{
    int maxS, maxT, viewS, viewT;
    long droneFixS, droneFixT, droneIncS, droneIncT;
    int droneS, droneT, dronePrevS, dronePrevT, droneMapX, droneMapY, dronePrevMapX, dronePrevMapY;
    int droneWidth, droneHeight, sizeofDrone, droneSpeed;
    long missileFixS, missileFixT, missileIncS, missileIncT;
    int missileS, missileT, missileWidth, missileHeight, missileInFlight, sizeofMissile;
    int missileMapX, missileMapY, missilePrevMapX, missilePrevMapY;
    long samFixS, samFixT, samIncS, samIncT;
    int samS, samT, samPrevS, samPrevT, samWidth, samHeight, samInFlight, sizeofSAM;
    int fireballSeq, fireballWidth, fireballHeight, sizeofFireball;
    unsigned char far *drone, far *missile, far *sam, far *fireball;
    unsigned char droneAngle, droneDir, samDir, ending, explosion, quit, singlestep;
    int diffS, diffT, viewOffsetS, viewOffsetT;
    unsigned long st;
    int scrolldir, i;

    if (!spriteLoad("drone.spr", &drone, &droneWidth, &droneHeight))
    {
        viewExit();
        txt80();
        fprintf(stderr, "Unable to load drone sprite page\n");
        exit(1);
    }
    sizeofDrone = droneWidth * droneHeight / 2;
    if (!spriteLoad("missile.spr", &missile, &missileWidth, &missileHeight))
    {
        viewExit();
        txt80();
        fprintf(stderr, "Unable to load missile sprite page\n");
        exit(1);
    }
    sizeofMissile = missileWidth * missileHeight / 2;
    if (!spriteLoad("sam.spr", &sam, &samWidth, &samHeight))
    {
        viewExit();
        txt80();
        fprintf(stderr, "Unable to load SAM sprite page\n");
        exit(1);
    }
    sizeofSAM = samWidth * samHeight / 2;
    if (!(fireballSeq = spriteLoad("fireball.spr", &fireball, &fireballWidth, &fireballHeight)))
    {
        viewExit();
        txt80();
        fprintf(stderr, "Unable to load missile sprite page\n");
        exit(1);
    }
    sizeofFireball = fireballWidth * fireballHeight / 2;
    if ((tileCount = tilesetLoad("warzone.set", (unsigned char far * *)&tileset, sizeof(struct tile))) == 0)
    {
        viewExit();
        txt80();
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
    mapWidth        = st;
    mapHeight       = st >> 16;
    maxS             = mapWidth << 4;
    maxT            = mapHeight << 4;
    viewOffsetS     = 80 - droneWidth / 2;
    viewOffsetT     = 50 - droneHeight / 2;
    droneFixS       = (long)maxS << 15;
    droneFixT       = (long)maxT << 15;
    droneS          = (int)(droneFixS >> 16) & 0xFFFE;
    droneT          = (int)(droneFixT >> 16) & 0xFFFE;
    dronePrevS      = droneS;
    dronePrevT      = droneT;
    dronePrevMapX   =
    dronePrevMapY   = 0;
    droneAngle      =
    droneDir        = 0;
    droneSpeed      = MIN_SPEED;
    droneIncS       = cosFix[droneDir] / droneSpeed;
    droneIncT       = sinFix[droneDir]   / droneSpeed;
    viewS           = (droneS - viewOffsetS) & 0xFFFE;
    viewT           = (droneT - viewOffsetT) & 0xFFFE;
    explosion       =
    samInFlight     =
    missileInFlight = 0;
    singlestep      =
    ending          =
    quit            = 0;
    viewInit(gr160(BLACK, BROWN), viewS, viewT, mapWidth, mapHeight, (unsigned char far * far *)tilemap);
    spriteEnable(0, droneS, droneT, droneWidth, droneHeight, drone + droneDir * sizeofDrone);
    SET_SOUND(droneBuzz[droneSpeed]);
    START_SOUND;
    do
    {
#ifdef PROFILE
        rasterBorder(WHITE);
#endif
        scrolldir = 0;
        /*
         * Update drone position
         */
        droneFixS += droneIncS;
        droneS     = (int)(droneFixS >> 16) & 0xFFFE;
        droneMapX  = droneS >> 4;
        droneFixT += droneIncT;
        droneT     = (int)(droneFixT >> 16) & 0xFFFE;
        droneMapY  = droneT >> 4;
        if (droneS != dronePrevS || droneT != dronePrevT)
        {
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
            /*
             * Bounce drone off map boundaries for now
             */
            if (droneS < 0 || droneS > maxS - droneWidth || droneT < 0 || droneT > maxT - droneHeight)
            {
                droneAngle ^= 0x80;
                droneDir  = droneAngle >> 4;
                droneIncS = cosFix[droneDir] / droneSpeed;
                droneIncT = sinFix[droneDir]   / droneSpeed;
                spriteUpdate(0, drone + droneDir * sizeofDrone);
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
                            /*
                             * Create explosion
                             */
                            explosion = 1;
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
                            ending    = 1;
                            explosion = 1;
                            samInFlight = 0;
                            spriteDisable(0);
                            spriteDisable(2);
                            spriteEnable(3, samS + (samWidth - fireballWidth)/2, samT + (samHeight - fireballHeight)/2, fireballWidth, fireballHeight, fireball);
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
                switch (getkb())
                {
                    case UP_ARROW: // Speed up
                        if (droneSpeed > MAX_SPEED)
                        {
                            droneSpeed--;
                            droneIncS = cosFix[droneDir] / droneSpeed;
                            droneIncT = sinFix[droneDir] / droneSpeed;
                        }
                        break;
                    case DOWN_ARROW: // Slow down
                        if (droneSpeed < MIN_SPEED)
                        {
                            droneSpeed++;
                            droneIncS = cosFix[droneDir] / droneSpeed;
                            droneIncT = sinFix[droneDir] / droneSpeed;
                        }
                        break;
                    case LEFT_ARROW: // Turn left
                        droneAngle -= 16;
                        if ((droneAngle >> 4) != droneDir)
                        {
                            droneDir   = droneAngle >> 4;
                            droneIncS  = cosFix[droneDir] / droneSpeed;
                            droneIncT  = sinFix[droneDir] / droneSpeed;
                            droneFixS &= 0xFFFE0000L;
                            droneFixT &= 0xFFFE0000L;
                            spriteUpdate(0, drone + droneDir * sizeofDrone);
                        }
                        break;
                    case RIGHT_ARROW: // Turn right
                        droneAngle += 16;
                        if ((droneAngle >> 4) != droneDir)
                        {
                            droneDir   = droneAngle >> 4;
                            droneIncS  = cosFix[droneDir] / droneSpeed;
                            droneIncT  = sinFix[droneDir] / droneSpeed;
                            droneFixS &= 0xFFFE0000L;
                            droneFixT &= 0xFFFE0000L;
                            spriteUpdate(0, drone + droneDir * sizeofDrone);
                        }
                        break;
                    case SPACEBAR: // Fire missile
                        if (!missileInFlight)
                        {
                            missileIncS = (cosFix[droneDir] << 1) + droneIncS;
                            missileFixS = droneFixS + missileIncS + ((long)(droneWidth - missileWidth) << 15);
                            missileIncT = (sinFix[droneDir] << 1) + droneIncT;
                            missileFixT = droneFixT + missileIncT + ((long)(droneHeight - missileHeight) << 15);
                            if (spriteEnable(1, missileFixS >> 16, missileFixT >> 16, missileWidth, missileHeight, missile + droneDir * sizeofMissile))
                            {
                                missilePrevMapX = 0;
                                missilePrevMapY = 0;
                                missileInFlight = MISSILE_FLIGHT_TIME;
                            }
                        }
                        break;
                    case 's':
                        singlestep = 1;
                        break;
                    case ESCAPE: // Quit
                        quit = 1;
                        break;
                }
                break;
            case 1: // Sound sequencing
                if (explosion)
                    SET_SOUND(explodeNoise[explosion - 1]);
                else if (samInFlight)
                    SET_SOUND(samAlert[samInFlight & 0x03]);
                else if (missileInFlight)
                    SET_SOUND(missileWhiz[missileInFlight & 0x03]);
                else
                    SET_SOUND(droneBuzz[droneSpeed] + (frameCount & 0x0010));
                break;
            case 2: // Explosion animation
                if (explosion)
                {
                    if (explosion == fireballSeq)
                    {
                        explosion = 0;
                        spriteDisable(3);
                        if (ending)
                            quit = 1;
                    }
                    else
                        spriteUpdate(3, fireball + explosion++ * sizeofFireball);
                }
                break;
            case 3: // Enemy AI
                if (samInFlight)
                {
                    i = samDir;
                    samDir = xy2angle(samS - droneS, samT - droneT);
                    if (i != samDir)
                    {
                        samIncS = cosFix[samDir];
                        samIncT = sinFix[samDir];
                        spriteUpdate(2, sam + samDir * sizeofSAM);
                    }
                }
                else if (droneMapX != dronePrevMapX || droneMapY != dronePrevMapY)
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
                                /*
                                 * Launch SAM at drone
                                 */
                                samFixS = ((long)(droneMapX - SAM_RANGE) << 20) + ((long)(16 - samWidth) << 15);
                                samFixT = ((long)i << 20) + ((long)(16 - samHeight) << 15);
                                samDir  = xy2angle((int)(samFixS >> 16) - droneS, (int)(samFixT >> 16) - droneT);
                                samIncS = cosFix[samDir];
                                samIncT = sinFix[samDir];
                                if (spriteEnable(2, samFixS >> 16, samFixT >> 16, samWidth, samHeight, sam + samDir * sizeofSAM))
                                {
                                    samPrevS    = 0;
                                    samPrevT    = 0;
                                    samInFlight = SAM_FLIGHT_TIME;
                                }
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
                                /*
                                 * Launch SAM at drone
                                 */
                                samFixS = ((long)(droneMapX + SAM_RANGE) << 20) + ((long)(16 - samWidth) << 15);
                                samFixT = ((long)i << 20) + ((long)(16 - samHeight) << 15);
                                samDir  = xy2angle((int)(samFixS >> 16) - droneS, (int)(samFixT >> 16) - droneT);
                                samIncS = cosFix[samDir];
                                samIncT = sinFix[samDir];
                                if (spriteEnable(2, samFixS >> 16, samFixT >> 16, samWidth, samHeight, sam + samDir * sizeofSAM))
                                {
                                    samPrevS    = 0;
                                    samPrevT    = 0;
                                    samInFlight = SAM_FLIGHT_TIME;
                                }
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
                                    /*
                                     * Launch SAM at drone
                                     */
                                    samFixS = ((long)i << 20) + ((long)(16 - samWidth) << 15);
                                    samFixT = ((long)(droneMapY - SAM_RANGE) << 20) + ((long)(16 - samHeight) << 15);
                                    samDir  = xy2angle((int)(samFixS >> 16) - droneS, (int)(samFixT >> 16) - droneT);
                                    samIncS = cosFix[samDir];
                                    samIncT = sinFix[samDir];
                                    if (spriteEnable(2, samFixS >> 16, samFixT >> 16, samWidth, samHeight, sam + samDir * sizeofSAM))
                                    {
                                        samPrevS    = 0;
                                        samPrevT    = 0;
                                        samInFlight = SAM_FLIGHT_TIME;
                                    }
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
                                    /*
                                     * Launch SAM at drone
                                     */
                                    samFixS = ((long)i << 20) + ((long)(16 - samWidth) << 15);
                                    samFixT = ((long)(droneMapY + SAM_RANGE) << 20) + ((long)(16 - samHeight) << 15);
                                    samDir  = xy2angle((int)(samFixS >> 16) - droneS, (int)(samFixT >> 16) - droneT);
                                    samIncS = cosFix[samDir];
                                    samIncT = sinFix[samDir];
                                    if (spriteEnable(2, samFixS >> 16, samFixT >> 16, samWidth, samHeight, sam + samDir * sizeofSAM))
                                    {
                                        samPrevS    = 0;
                                        samPrevT    = 0;
                                        samInFlight = SAM_FLIGHT_TIME;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                dronePrevMapX = droneMapX;
                dronePrevMapY = droneMapY;
                break;
        }
        dronePrevS = droneS;
        dronePrevT = droneT;
        st = viewRefresh(scrolldir);
        viewS  = st;
        viewT  = st >> 16;
        if (singlestep)
            singlestep = getch() == 's';
    } while (!quit);
    STOP_SOUND;
    viewExit();
    txt80();
    return 0;
}
