#include <dos.h>
#include <conio.h>
#include "lores.h"
#include "tiler.h"

extern unsigned int scanline[100]; // Precalculated scanline offsets
extern unsigned int scrnMask;
#ifdef PROFILE
extern unsigned char borderColor;
#endif
/*
 * Fast CGA routines
 */
void cpyEdgeH(int addr, int count);
void cpyEdgeV(int addr);
/*
 * Fast memory routines
 */
void tileMem(unsigned int s, unsigned int t, int width, int height, unsigned char far *tile, int span, unsigned char far *buf);
void tileEdgeH(unsigned int s, unsigned int t, unsigned char far * far*tileptr);
void tileEdgeH2(unsigned int s, unsigned int t, unsigned char far * far*tileptr);
void tileEdgeV(unsigned int s, unsigned int t, unsigned char far * far *tileptr);
void cpyBuf2Buf(int width, int height, int spanSrc, unsigned char far *bufSrc, int spanDst, unsigned char far *bufDst);
/*
 * Graphics routines for 160x100x16 color mode
 */
unsigned char edgeH[2][80], edgeV[100];
unsigned int orgAddr = 1;
unsigned int orgS = 0;
unsigned int orgT = 0;
unsigned int maxOrgS, maxOrgT, extS, extT;
unsigned int widthMapS, widthMapT, widthMap, spanMap, heightMap;
unsigned char far * far *tileMap;
/*
 * On-the-fly tile updates
 */
int                tileUpdateCount = 0;
unsigned int       tileUpdateS[16];
unsigned int       tileUpdateT[16];
unsigned char far *tileUpdatePtr[16];
/*
 * Sprite state
 */
#define STATE_INACTIVE      0x00
#define STATE_DISABLING     0x01
#define STATE_ACTIVE        0x02
#define STATE_MOVING        0x03
#define STATE_POSITIONING   0x04
#define ERASE_BORDER        4
/*
 * Sprite table
 */
struct sprite_t
{
    unsigned char far *spriteptr;   // Sprite image
    unsigned char far *spritebuf;   // Surrounding background+sprite
    unsigned char far *erasebuf;    // Erase previous sprite position
    unsigned int       s, bufS, eraS;
    unsigned int       t, bufT, eraT;
    int                width,  bufWidth, eraWidth;
    int                height, bufHeight;
    unsigned char      state;
} spriteTable[NUM_SPRITES];
/*
 * Update tile in map
 */
void tileUpdate(unsigned i, unsigned j, unsigned char far *tileNew)
{
    unsigned int s, t;

    *(tileMap + (j * widthMap) + i) = tileNew;
    s = i << 4;
    t = j << 4;
    /*
     * Check for on-screen update
     */
    if ((s < extS) && (s + 16 > orgS)
     && (t < extT) && (t + 16 > orgT))
    {
        if (tileUpdateCount < 16)
        {
            tileUpdatePtr[tileUpdateCount] = tileNew;
            tileUpdateS[tileUpdateCount]   = i << 4;
            tileUpdateT[tileUpdateCount]   = j << 4;
            tileUpdateCount++;
        }
    }
}
/*
 * Sprite routines
 */
void spriteEnable(int index, unsigned int s, unsigned int t, int width, int height, unsigned char far *spriteImg)
{
    spriteTable[index].state     = STATE_MOVING;
    spriteTable[index].spriteptr = spriteImg;
    spriteTable[index].spritebuf = (unsigned char *)malloc((width+ERASE_BORDER+1)/2*(height+ERASE_BORDER)); // Leave room for erase border
    spriteTable[index].erasebuf  = (unsigned char *)malloc((width+1/2)*height);
    spriteTable[index].width     = width;
    spriteTable[index].height    = height;
    spriteTable[index].s         = s;
    spriteTable[index].bufS      = s & 0xFFFE;
    spriteTable[index].t         = t;
    spriteTable[index].bufT      = t;
    spriteTable[index].bufWidth  = ((spriteTable[index].s + width + 1) & 0xFFFE) - spriteTable[index].bufS;
    spriteTable[index].bufHeight = height;
}
void spriteDisable(int index)
{
    unsigned int s;

    spriteTable[index].state    = STATE_DISABLING;
    spriteTable[index].eraS     = spriteTable[index].s & 0xFFFE;
    spriteTable[index].eraWidth = ((spriteTable[index].s + spriteTable[index].width + 1) & 0xFFFE) - spriteTable[index].eraS;
    spriteTable[index].eraT     = spriteTable[index].t;
}
void spriteUpdate(int index, unsigned char far *spriteImg)
{
    spriteTable[index].spriteptr = spriteImg;
    if (spriteTable[index].state == STATE_ACTIVE)
    {
        spriteTable[index].state     = STATE_MOVING;
        spriteTable[index].bufS      = spriteTable[index].s & 0xFFFE;
        spriteTable[index].bufT      = spriteTable[index].t;
        spriteTable[index].bufWidth  = ((spriteTable[index].s + spriteTable[index].width + 1) & 0xFFFE) - spriteTable[index].bufS;
        spriteTable[index].bufHeight = spriteTable[index].height;
    }
}
unsigned long spritePosition(int index, unsigned int s, unsigned int t)
{
    int deltaS, deltaT;
    struct sprite_t *sprite;

    sprite = &spriteTable[index];
    if (s > (widthMapS - sprite->width))
        s = widthMapS - sprite->width;
    if (t > (widthMapT - sprite->height))
        t = widthMapT - sprite->height;
    if (s != sprite->s || t != sprite->t || sprite->state == STATE_MOVING)
    {
        deltaS = s - sprite->s;
        deltaT = t - sprite->t;
        if ((deltaS <= ERASE_BORDER && deltaS >= -ERASE_BORDER) && (deltaT <= ERASE_BORDER && deltaT >= -ERASE_BORDER))
        {
            /*
             * More efficient to use STATE_MOVING()
             */
            sprite->state = STATE_MOVING;
            if (deltaS <= 0)
            {
                /*
                 * Create erase border to the right
                 */
                sprite->bufS     = s & 0xFFFE;
                sprite->bufWidth = ((sprite->s + sprite->width + 1) & 0xFFFE) - sprite->bufS;
            }
            else // deltaS > 0
                /*
                 * Create erase border to the left
                 */
                sprite->bufWidth = ((s + sprite->width + 1) & 0xFFFE) - sprite->bufS;
            if (deltaT <= 0)
            {
                /*
                 * Create erase border to the bottom
                 */
                sprite->bufT      = t;
                sprite->bufHeight = (sprite->t + sprite->height) - sprite->bufT;
            }
            else // deltaT > 0
                /*
                 * Create erase border to the top
                 */
                sprite->bufHeight = (t + sprite->width) - sprite->bufT;
            sprite->s     = s;
            sprite->t     = t;
        }
        else
        {
            /*
             * Erase at old position and draw at new position
             */
            sprite->state     = STATE_POSITIONING;
            sprite->eraS      = sprite->s & 0xFFFE;
            sprite->eraWidth  = ((sprite->s + sprite->width + 1) & 0xFFFE) - sprite->eraS;
            sprite->eraT      = sprite->t;
            sprite->bufS      = s & 0xFFFE;
            sprite->bufWidth  = ((s + sprite->width + 1) & 0xFFFE) - sprite->bufS;
            sprite->bufT      = t;
            sprite->bufHeight = sprite->height;
            sprite->s         = s;
            sprite->t         = t;
        }
    }
    return ((unsigned long)sprite->t << 16) | sprite->s;
}
void spriteIntersectRect(unsigned int leftS, unsigned int topT, int rectWidth, int rectHeight)
{
    unsigned int rightS, bottomT;
    struct sprite_t *sprite;

    rightS  = leftS + rectWidth  - 1;
    bottomT = topT  + rectHeight - 1;
    for (sprite = &spriteTable[0]; sprite < &spriteTable[NUM_SPRITES]; sprite++)
    {
        if ((sprite->state == STATE_ACTIVE)
         && (sprite->s < rightS)
         && ((sprite->s + sprite->width) > leftS)
         && (sprite->t < bottomT)
         && ((sprite->t + sprite->height) > topT))
        {
            sprite->state     = STATE_MOVING;
            sprite->bufS      = sprite->s & 0xFFFE;
            sprite->bufT      = sprite->t;
            sprite->bufWidth  = ((sprite->s + sprite->width + 1) & 0xFFFE) - sprite->bufS;
            sprite->bufHeight = sprite->height;
        }
    }
}
/*
 * Update update buffer with lower priority sprites that intersect
 */
void spriteIntersectSpriteBuf(struct sprite_t *spriteAbove)
{
    int leftS, rightS, topT, bottomT;
    int width, height, spanAbove, spanBelow;
    struct sprite_t *sprite;
    unsigned char far *bufAbove;
    unsigned char far *bufBelow;

    leftS   = spriteAbove->bufS;
    rightS  = leftS + spriteAbove->bufWidth;
    topT    = spriteAbove->bufT;
    bottomT = topT  + spriteAbove->bufHeight;
    for (sprite = &spriteTable[0]; sprite != spriteAbove; sprite++)
    {
        if ((sprite->state >= STATE_ACTIVE)
         && (sprite->s < rightS)
         && ((sprite->s + sprite->width) > leftS)
         && (sprite->t < bottomT)
         && ((sprite->t + sprite->height) > topT))
        {
            if (spriteAbove->state == STATE_ACTIVE)
            {
                /*
                 * Moving sprite underneath non-moving sprite, so update state to moving
                 */
                spriteAbove->state = STATE_MOVING;
                tileBuf(spriteAbove->bufS, spriteAbove->bufT, spriteAbove->bufWidth, spriteAbove->bufHeight, spriteAbove->spritebuf);
            }
            /*
             * Clip sprite buffer to sprite buffer above
             */
            height    = spriteAbove->bufHeight;
            width     = spriteAbove->bufWidth;
            spanAbove = spriteAbove->bufWidth >> 1;
            bufAbove  = spriteAbove->spritebuf;
            spanBelow = sprite->bufWidth >> 1;
            bufBelow  = sprite->spritebuf;
            if ((leftS - (int)sprite->bufS) > 0)
            {
                bufBelow += (leftS - (int)sprite->bufS) >> 1;
            }
            else if (((int)sprite->bufS - leftS) > 0)
            {
                width    -=  (int)sprite->bufS - leftS;
                bufAbove += ((int)sprite->bufS - leftS) >> 1;
            }
            if ((rightS - ((int)sprite->bufS + sprite->bufWidth)) > 0)
            {
                width -= rightS - ((int)sprite->bufS + sprite->bufWidth);
            }
            if ((topT - (int)sprite->bufT) > 0)
            {
                bufBelow += (topT - (int)sprite->bufT) * spanBelow;
            }
            else if (((int)sprite->bufT - topT) > 0)
            {
                height   -=  (int)sprite->bufT - topT;
                bufAbove += ((int)sprite->bufT - topT) * spanAbove;
            }
            if ((bottomT - ((int)sprite->bufT + sprite->bufHeight)) > 0)
            {
                height -= bottomT - ((int)sprite->bufT + sprite->bufHeight);
            }
            cpyBuf2Buf(width, height, spanBelow, bufBelow, spanAbove, bufAbove);
        }
    }
}
void spriteIntersectEraseBuf(struct sprite_t *spriteAbove)
{
    int leftS, rightS, topT, bottomT;
    int width, height, spanAbove, spanBelow;
    struct sprite_t *sprite;
    unsigned char far *bufAbove;
    unsigned char far *bufBelow;

    leftS   = spriteAbove->eraS;
    rightS  = leftS + spriteAbove->eraWidth  - 1;
    topT    = spriteAbove->eraT;
    bottomT = topT  + spriteAbove->height - 1;
    for (sprite = &spriteTable[0]; sprite != spriteAbove; sprite++)
    {
        if ((sprite->state >= STATE_ACTIVE)
         && (sprite->s < rightS)
         && ((sprite->s + sprite->width) > leftS)
         && (sprite->t < bottomT)
         && ((sprite->t + sprite->height) > topT))
        {
            /*
             * Clip sprite buffer to erase buffer above
             */
            height    = spriteAbove->height;
            width     = spriteAbove->eraWidth;
            spanAbove = spriteAbove->eraWidth >> 1;
            bufAbove  = spriteAbove->erasebuf;
            spanBelow = sprite->bufWidth >> 1;
            bufBelow  = sprite->spritebuf;
            if ((leftS - (int)sprite->bufS) > 0)
            {
                bufBelow += (leftS - (int)sprite->bufS) >> 1;
            }
            else if (((int)sprite->bufS - leftS) > 0)
            {
                width    -=  (int)sprite->bufS - leftS;
                bufAbove += ((int)sprite->bufS - leftS) >> 1;
            }
            if ((rightS - ((int)sprite->bufS + sprite->bufWidth)) > 0)
            {
                width -= rightS - ((int)sprite->bufS + sprite->bufWidth);
            }
            if ((topT - (int)sprite->bufT) > 0)
            {
                bufBelow += (topT - (int)sprite->bufT) * spanBelow;
            }
            else if (((int)sprite->bufT - topT) > 0)
            {
                height   -=  (int)sprite->bufT - topT;
                bufAbove += ((int)sprite->bufT - topT) * spanAbove;
            }
            if ((bottomT - ((int)sprite->bufT + sprite->bufHeight)) > 0)
            {
                height -= bottomT - ((int)sprite->bufT + sprite->bufHeight);
            }
            cpyBuf2Buf(width, height, spanBelow, bufBelow, spanAbove, bufAbove);
        }
    }
}
/*
 * Refresh tile and sprite view
 */
unsigned long viewScroll(int scrolldir)
{
    unsigned int hcount, haddr, vaddr, i;
    struct sprite_t *sprite;

#ifdef PROFILE
    rasterBorder(GREEN);
#endif
    if (scrolldir & SCROLL_LEFT2)
    {
        if (orgS < maxOrgS)
        {
            orgS   += 2;
            extS    = orgS + 160;
            orgAddr = (orgAddr + 2) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_LEFT2 | SCROLL_RIGHT2);
    }
    else if (scrolldir & SCROLL_RIGHT2)
    {
        if (orgS > 0)
        {
            orgS    = (orgS - 2) & 0x0FFFE;
            extS    = orgS + 160;
            orgAddr = (orgAddr - 2) & 0x3FFF;
        }
        else
        scrolldir &= ~(SCROLL_LEFT2 | SCROLL_RIGHT2);
    }
    if (scrolldir & SCROLL_UP2)
    {
        if (orgT < maxOrgT)
        {
            orgT    = (orgT + 2) & 0x0FFFE;
            extT    = orgT + 100;
            orgAddr = (orgAddr + 320) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    else if (scrolldir & SCROLL_DOWN2)
    {
        if (orgT > 0)
        {
            orgT    = (orgT - 2) & 0x0FFFE;
            extT    = orgT + 100;
            orgAddr = (orgAddr - 320) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    else if (scrolldir & SCROLL_UP)
    {
        if (orgT < maxOrgT + 1)
        {
            orgT++;
            extT    = orgT + 100;
            orgAddr = (orgAddr + 160) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    else if (scrolldir & SCROLL_DOWN)
    {
        if (orgT > 0)
        {
            orgT--;
            extT    = orgT + 100;
            orgAddr = (orgAddr - 160) & 0x3FFF;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    /*
     * Pre-render edges based on scroll direction
     */
    if (scrolldir & SCROLL_RIGHT2)
    {
        /*
         * Fill in left edge
         */
        vaddr = orgAddr;
        tileEdgeV(orgS & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * widthMap + (orgS >> 4));
        spriteIntersectRect(orgS, orgT, 2, 100);
    }
    else if (scrolldir & SCROLL_LEFT2)
    {
        /*
         * Fill right edge
         */
        vaddr = (orgAddr + 158) & 0x3FFF;
        tileEdgeV((orgS + 158) & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * widthMap + ((orgS + 158) >> 4));
        spriteIntersectRect(orgS + 158, orgT, 2, 100);
    }
    if (scrolldir & SCROLL_DOWN2)
    {
        /*
         * Fill in top edge
         */
        hcount = 2;
        haddr  = orgAddr;
        tileEdgeH2(orgS & 0x0E, orgT & 0x0E, tileMap + (orgT >> 4) * widthMap + (orgS >> 4));
        spriteIntersectRect(orgS, orgT, 160, 2);
    }
    else if (scrolldir & SCROLL_UP2)
    {
        /*
         * Fill in botom edge
         */
        hcount = 2;
        haddr  = (orgAddr + 98 * 160) & 0x3FFF;
        tileEdgeH2(orgS & 0x0E, (orgT + 98) & 0x0E, tileMap + ((orgT + 98) >> 4) * widthMap + (orgS >> 4));
        spriteIntersectRect(orgS, orgT + 98, 160, 2);
    }
    else if (scrolldir & SCROLL_DOWN)
    {
        /*
         * Fill in top edge
         */
        haddr  = orgAddr;
        hcount = 1;
        tileEdgeH(orgS & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * widthMap + (orgS >> 4));
        spriteIntersectRect(orgS, orgT, 160, 1);
    }
    else if (scrolldir & SCROLL_UP)
    {
        /*
         * Fill in botom edge
         */
        haddr  = (orgAddr + 99 * 160) & 0x3FFF;
        hcount = 1;
        tileEdgeH(orgS & 0x0E, (orgT + 99) & 0x0F, tileMap + ((orgT + 99) >> 4) * widthMap + (orgS >> 4));
        spriteIntersectRect(orgS, orgT + 99, 160, 1);
    }
    /*
     * Intersect tile updates with active sprites that may need redraw
     */
    for (i = 0; i < tileUpdateCount; i++)
    {
        if ((tileUpdateS[i] < extS) && (tileUpdateS[i] + 16 > orgS)
         && (tileUpdateT[i] < extT) && (tileUpdateT[i] + 16 > orgT))
            spriteIntersectRect(tileUpdateS[i], tileUpdateT[i], 16, 16);
    }
    /*
     * Update sprite buffers
     */
    for (sprite = &spriteTable[0]; sprite != &spriteTable[NUM_SPRITES]; sprite++)
    {
        if (sprite->state)
        {
            if (sprite->state == STATE_ACTIVE)
            {
                sprite->bufS      = sprite->s & 0xFFFE;
                sprite->bufWidth  = ((sprite->s + sprite->width + 1) & 0xFFFE) - sprite->bufS;
                sprite->bufT      = sprite->t;
                sprite->bufHeight = sprite->height;
                spriteIntersectSpriteBuf(sprite);
                /*
                 * State could get updated if other ananosprite moving underneath
                 */
                if (sprite->state == STATE_MOVING)
                    spriteBuf(sprite->s - sprite->bufS, sprite->t - sprite->bufT, sprite->width, sprite->height, sprite->spriteptr, sprite->bufWidth >> 1, sprite->spritebuf);
            }
            else if (sprite->state == STATE_MOVING)
            {
                tileBuf(sprite->bufS, sprite->bufT, sprite->bufWidth, sprite->bufHeight, sprite->spritebuf);
                spriteIntersectSpriteBuf(sprite);
                spriteBuf(sprite->s - sprite->bufS, sprite->t - sprite->bufT, sprite->width, sprite->height, sprite->spriteptr, sprite->bufWidth >> 1, sprite->spritebuf);
            }
            else if (sprite->state == STATE_POSITIONING)
            {
                tileBuf(sprite->eraS, sprite->eraT, sprite->eraWidth, sprite->height, sprite->erasebuf);
                spriteIntersectEraseBuf(sprite);
                tileBuf(sprite->bufS, sprite->bufT, sprite->bufWidth, sprite->bufHeight, sprite->spritebuf);
                spriteIntersectSpriteBuf(sprite);
                spriteBuf(sprite->s - sprite->bufS, 0, sprite->width, sprite->height, sprite->spriteptr, sprite->bufWidth >> 1, sprite->spritebuf);
            }
            else if (sprite->state == STATE_DISABLING)
            {
                tileBuf(sprite->eraS, sprite->eraT, sprite->eraWidth, sprite->height, sprite->erasebuf);
                spriteIntersectEraseBuf(sprite);
            }
        }
    }
#ifdef PROFILE
    rasterBorder(borderColor);
#endif
    /*
     * The following happens after last active scanline
     */
    setStartAddr(orgAddr >> 1);
#ifdef PROFILE
    rasterBorder(RED);
#endif
    /*
     * Fill in edges
     */
    if (scrolldir & (SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN))
        cpyEdgeH(haddr, hcount);
    if (scrolldir & (SCROLL_LEFT2 | SCROLL_RIGHT2))
        cpyEdgeV(vaddr);
    /*
     * Draw any updated tiles
     */
    while (tileUpdateCount)
    {
        tileUpdateCount--;
        cpyBuf(tileUpdateS[tileUpdateCount], tileUpdateT[tileUpdateCount], 16, 16, tileUpdatePtr[tileUpdateCount]);
    }
    /*
     * Update sprites
     */
    for (sprite = &spriteTable[0]; sprite < &spriteTable[NUM_SPRITES]; sprite++)
    {
        if (sprite->state)
        {
            if (sprite->state == STATE_MOVING)
            {
                cpyBuf(sprite->bufS, sprite->bufT, sprite->bufWidth, sprite->bufHeight, sprite->spritebuf);
                sprite->state = STATE_ACTIVE;
                sprite->bufS  = sprite->s & 0xFFFE;
                sprite->bufT  = sprite->t;
            }
            else if (sprite->state == STATE_POSITIONING)
            {
                cpyBuf(sprite->eraS, sprite->eraT, sprite->eraWidth, sprite->height,    sprite->erasebuf);
                cpyBuf(sprite->bufS, sprite->bufT, sprite->bufWidth, sprite->bufHeight, sprite->spritebuf);
                sprite->state = STATE_ACTIVE;
            }
            else if (sprite->state == STATE_DISABLING)
            {
                cpyBuf(sprite->eraS, sprite->eraT, sprite->eraWidth, sprite->height, sprite->erasebuf);
                free(sprite->spritebuf);
                free(sprite->erasebuf);
                sprite->state     = STATE_INACTIVE;
                sprite->spriteptr = 0L;
                sprite->spritebuf = 0L;
                sprite->erasebuf  = 0L;
            }
        }
    }
#ifdef PROFILE
    rasterBorder(borderColor);
#endif
    /*
     * Return updated origin as 32 bit value
     */
    return ((unsigned long)orgT << 16) | orgS;
}
/*
 * Use hardware double buffering and redraw entire frame
 */
unsigned long viewRedraw(int scrolldir)
{
    struct sprite_t *sprite;
    unsigned char far *spriteImg;
    int spriteX, spriteY, spriteWidth, spriteHeight;

    if (scrolldir & SCROLL_LEFT2)
    {
        if (orgS < maxOrgS)
        {
            orgS += 2;
            extS  = orgS + 160;
        }
        else
            scrolldir &= ~(SCROLL_LEFT2 | SCROLL_RIGHT2);
    }
    else if (scrolldir & SCROLL_RIGHT2)
    {
        if (orgS > 0)
        {
            orgS = (orgS - 2) & 0x0FFFE;
            extS = orgS + 160;
        }
        else
        scrolldir &= ~(SCROLL_LEFT2 | SCROLL_RIGHT2);
    }
    if (scrolldir & SCROLL_UP2)
    {
        if (orgT < maxOrgT)
        {
            orgT = (orgT + 2) & 0x0FFFE;
            extT = orgT + 100;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    else if (scrolldir & SCROLL_DOWN2)
    {
        if (orgT > 0)
        {
            orgT = (orgT - 2) & 0x0FFFE;
            extT = orgT + 100;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    else if (scrolldir & SCROLL_UP)
    {
        if (orgT < maxOrgT + 1)
        {
            orgT++;
            extT = orgT + 100;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    else if (scrolldir & SCROLL_DOWN)
    {
        if (orgT > 0)
        {
            orgT--;
            extT = orgT + 100;
        }
        else
            scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    }
    /*
     * Draw tiles
     */
    tileScrn(orgS, orgT);
    /*
     * Draw sprites
     */
    for (sprite = &spriteTable[0]; sprite < &spriteTable[NUM_SPRITES]; sprite++)
    {
        if ((sprite->state >= STATE_ACTIVE)
         && (sprite->s < extS)
         && ((sprite->s + sprite->width) > orgS)
         && (sprite->t < extT)
         && ((sprite->t + sprite->height) > orgT))
        {
            spriteX      = sprite->s - orgS;
            spriteY      = sprite->t - orgT;
            spriteWidth  = sprite->width;
            spriteHeight = sprite->height;
            spriteImg    = sprite->spriteptr;
            if (spriteX < 0)
            {
                spriteImg   += spriteX >> 1;
                spriteWidth += spriteX;
                spriteX      = 0;
            }
            if (spriteX + spriteWidth > 160)
            {
                spriteWidth = 160 - spriteX;
            }
            if (spriteY < 0)
            {
                spriteImg    += (-spriteY * sprite->width) >> 1;
                spriteHeight += spriteY;
                spriteY       = 0;
            }
            if (spriteY + spriteHeight > 100)
            {
                spriteHeight = 100 - spriteY;
            }
            spriteScrn(spriteX, spriteY, spriteWidth, spriteHeight, sprite->width >> 1, spriteImg);
        }
    }
    /*
     * Wait until VBlank and update CRTC start address to swap buffers
     */
    while (inp(0x3DA) & 0x08); // Wait until the end of VBlank
    outpw(0x3D4, ((orgAddr >> 1) & 0xFF00) + 12);
    while (!(inp(0x3DA) & 0x08)); // Wait until beginning of VBlank
    /*
     * Point orgAddr to back buffer
     */
    orgAddr ^= 0x4000;
    frameCount++;
    /*
     * Return updated origin as 32 bit value
     */
    return ((unsigned long)orgT << 16) | orgS;
}
void viewInit(int mode, unsigned int s, unsigned int t, unsigned int width, unsigned int height, unsigned char far * far *map)
{
    int i;
    /*
     * Init tile map
     */
    tileMap   = map;
    widthMap  = width;
    spanMap   = widthMap << 2;
    heightMap = height;
    widthMapS = width  << 4;
    widthMapT = height << 4;
    maxOrgS   = widthMapS - 160;
    maxOrgT   = widthMapT - 100;
    orgS      = s & 0xFFFE; // S always even
    orgT      = t;
    extS      = orgS + 160;
    extT      = orgT + 100;
    if (mode == REFRESH_SCROLL)
    {
        orgAddr = (orgT * 160 + orgS | 1) & 0x3FFF;
        viewRefresh = viewScroll;
        rasterDisable(); /* Turn off video */
        tileScrn(orgS, orgT);
        rasterEnable();  /* Turn on video */
        enableRasterTimer(199);
        setStartAddr(orgAddr >> 1);
    }
    else
    {
        orgAddr = 0x0001; // Draw to front buffer
        viewRefresh = viewRedraw;
        tileScrn(orgS, orgT);
        outpw(0x3D4, 0x0000 + 12);
        orgAddr = 0x4001; // Draw to back buffer
    }
    /*
     * Init sprite table
     */
    for (i = 0; i < NUM_SPRITES; i++)
        spriteTable[i].state = STATE_INACTIVE;
}
void viewExit(void)
{
    int i;
    /*
     * Clean up sprite table
     */
    for (i = 0; i < NUM_SPRITES; i++)
    {
        if (spriteTable[i].state)
        {
            free(spriteTable[i].spritebuf);
            free(spriteTable[i].erasebuf);
            spriteTable[i].spriteptr = 0L;
            spriteTable[i].spritebuf = 0L;
            spriteTable[i].erasebuf  = 0L;
            spriteTable[i].state         = STATE_INACTIVE;
        }
    }
    disableRasterTimer();
}
