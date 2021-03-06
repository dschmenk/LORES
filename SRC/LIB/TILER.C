#include <dos.h>
#include <conio.h>
#include <malloc.h>
#include "lores.h"
#include "tiler.h"
extern unsigned int scanline[100]; // Precalculated scanline offsets
extern unsigned int orgAddr;
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
void tileEdgeH(unsigned int s, unsigned int t, unsigned char far * far*tileptr);
void tileEdgeH2(unsigned int s, unsigned int t, unsigned char far * far*tileptr);
void tileEdgeV(unsigned int s, unsigned int t, unsigned char far * far *tileptr);
void cpyBuf2Buf(int width, int height, int spanSrc, unsigned char far *bufSrc, int spanDst, unsigned char far *bufDst);
/*
 * Graphics routines for 160x100x16 color mode
 */
unsigned char edgeH[2][80], edgeV[100];
unsigned int orgS = 0;
unsigned int orgT = 0;
unsigned int maxOrgS, maxOrgT, extS, extT;
unsigned int widthMapS, heightMapT, widthMap, spanMap, heightMap;
unsigned char far * far *tileMap;
unsigned long (*viewRefresh)(int scrolldir);
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
#define ERASE_BORDER        8
/*
 * Sprite table
 */
struct sprite_t
{
    unsigned char      state;
    int                width,  bufWidth,  eraWidth;
    int                height, bufHeight, eraHeight;
    unsigned char far *spriteptr;   // Sprite image
    unsigned char     *spritebuf;   // Surrounding background+sprite
    unsigned char     *erasebuf;    // Erase previous sprite position
    unsigned int       s, bufS, eraS;
    unsigned int       t, bufT, eraT;
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
int spriteEnable(int index, int s, int t, int width, int height, unsigned char far *spriteImg)
{
    struct sprite_t *sprite;

    sprite = &spriteTable[index];
    if (sprite->state == STATE_INACTIVE)
    {
        if (s < 0)
            s = 0;
        else if (s > (widthMapS - width))
            s = widthMapS - width;
        if (t < 0)
            t = 0;
        else if (t > (heightMapT - height))
            t = heightMapT - height;
        sprite->spriteptr = spriteImg;
        sprite->spritebuf = (unsigned char *)malloc((width+ERASE_BORDER+2)/2*(height+ERASE_BORDER)); // Leave room for erase border
        sprite->erasebuf  = (unsigned char *)malloc((width+ERASE_BORDER+2)/2*(height+ERASE_BORDER));
        sprite->width     = width;
        sprite->height    = height;
        sprite->s         = s;
        sprite->bufS      = s & 0xFFFE;
        sprite->t         = t;
        sprite->bufT      = t;
        sprite->bufWidth  = ((sprite->s + width + 1) & 0xFFFE) - sprite->bufS;
        sprite->bufHeight = height;
        if (sprite->spriteptr && sprite->spritebuf && sprite->erasebuf)
            sprite->state = STATE_MOVING;
        else
        {
            if (sprite->spritebuf)
                free(sprite->spritebuf);
            if (sprite->erasebuf)
                free(sprite->erasebuf);
            sprite->spriteptr = 0L;
            sprite->spritebuf = 0L;
            sprite->erasebuf  = 0L;
        }
    }
    return sprite->state == STATE_MOVING;
}
void spriteDisable(int index)
{
    struct sprite_t *sprite;

    sprite = &spriteTable[index];
    if (sprite->state >= STATE_ACTIVE)
    {
        sprite->state    = STATE_DISABLING;
        sprite->eraS      = sprite->bufS;
        sprite->eraWidth  = sprite->bufWidth;
        sprite->eraT      = sprite->bufT;
        sprite->eraHeight = sprite->bufHeight;
    }
}
void spriteUpdate(int index, unsigned char far *spriteImg)
{
    struct sprite_t *sprite;

    sprite = &spriteTable[index];
    sprite->spriteptr = spriteImg;
    if (sprite->state == STATE_ACTIVE)
    {
        sprite->state     = STATE_MOVING;
        sprite->bufS      = sprite->s & 0xFFFE;
        sprite->bufT      = sprite->t;
        sprite->bufWidth  = ((sprite->s + sprite->width + 1) & 0xFFFE) - sprite->bufS;
        sprite->bufHeight = sprite->height;
    }
}
unsigned long spritePosition(int index, int s, int t)
{
    int deltaS, deltaT;
    struct sprite_t *sprite;

    sprite = &spriteTable[index];
    if (sprite->state >= STATE_ACTIVE)
    {
        if (s < 0)
            s = 0;
        else if (s > (widthMapS - sprite->width))
            s = widthMapS - sprite->width;
        if (t < 0)
            t = 0;
        else if (t > (heightMapT - sprite->height))
            t = heightMapT - sprite->height;
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
                {
                    /*
                     * Create erase border to the left
                     */
                    sprite->bufS     = sprite->s & 0xFFFE;
                    sprite->bufWidth = ((s + sprite->width + 1) & 0xFFFE) - sprite->bufS;
                }
                if (deltaT <= 0)
                {
                    /*
                     * Create erase border to the bottom
                     */
                    sprite->bufT      = t;
                    sprite->bufHeight = (sprite->t + sprite->height) - sprite->bufT;
                }
                else // deltaT > 0
                {
                    /*
                     * Create erase border to the top
                     */
                    sprite->bufT      = sprite->t;
                    sprite->bufHeight = (t + sprite->height) - sprite->bufT;
                }
                sprite->s = s;
                sprite->t = t;
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
                sprite->eraHeight = sprite->height;
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
    return 0L;
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
    int deltaS, deltaT;
    int width, height, spanAbove, spanBelow;
    struct sprite_t *sprite;
    unsigned char far *bufAbove;
    unsigned char far *bufBelow;

    leftS   = spriteAbove->bufS;
    rightS  = leftS + spriteAbove->bufWidth;
    topT    = spriteAbove->bufT;
    bottomT = topT  + spriteAbove->bufHeight;
    for (sprite = &spriteTable[0]; sprite < spriteAbove; sprite++)
    {
        if ((sprite->state >= STATE_ACTIVE)
         && (sprite->bufS < rightS)
         && ((sprite->bufS + sprite->bufWidth) > leftS)
         && (sprite->bufT < bottomT)
         && ((sprite->bufT + sprite->bufHeight) > topT))
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
            deltaS    = leftS - (int)sprite->bufS;
            if (deltaS > 0)
                bufBelow += deltaS >> 1;
            else if (deltaS < 0)
            {
                width    +=  deltaS;
                bufAbove += -deltaS >> 1;
            }
            deltaS = rightS - ((int)sprite->bufS + sprite->bufWidth);
            if (deltaS > 0)
                width -= deltaS;
            deltaT = topT - (int)sprite->bufT;
            if (deltaT > 0)
                bufBelow += deltaT * spanBelow;
            else if (deltaT < 0)
            {
                height   +=  deltaT;
                bufAbove += -deltaT * spanAbove;
            }
            deltaT = bottomT - ((int)sprite->bufT + sprite->bufHeight);
            if (deltaT > 0)
                height -= deltaT;
            cpyBuf2Buf(width, height, spanBelow, bufBelow, spanAbove, bufAbove);
        }
    }
}
void spriteIntersectEraseBuf(struct sprite_t *spriteAbove)
{
    int leftS, rightS, topT, bottomT;
    int deltaS, deltaT;
    int width, height, spanAbove, spanBelow;
    struct sprite_t *sprite;
    unsigned char far *bufAbove;
    unsigned char far *bufBelow;

    leftS   = spriteAbove->eraS;
    rightS  = leftS + spriteAbove->eraWidth;
    topT    = spriteAbove->eraT;
    bottomT = topT  + spriteAbove->eraHeight;
    for (sprite = &spriteTable[0]; sprite < spriteAbove; sprite++)
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
            height    = spriteAbove->eraHeight;
            width     = spriteAbove->eraWidth;
            spanAbove = spriteAbove->eraWidth >> 1;
            bufAbove  = spriteAbove->erasebuf;
            spanBelow = sprite->bufWidth >> 1;
            bufBelow  = sprite->spritebuf;
            deltaS = leftS - (int)sprite->bufS;
            if (deltaS > 0)
                bufBelow += deltaS >> 1;
            else if (deltaS < 0)
            {
                width    +=  deltaS;
                bufAbove += -deltaS >> 1;
            }
            deltaS = rightS - ((int)sprite->bufS + sprite->bufWidth);
            if (deltaS > 0)
                width -= deltaS;
            deltaT = topT - (int)sprite->bufT;
            if (deltaT > 0)
                bufBelow += deltaT * spanBelow;
            else if (deltaT < 0)
            {
                height   +=  deltaT;
                bufAbove += -deltaT * spanAbove;
            }
            deltaT = bottomT - ((int)sprite->bufT + sprite->bufHeight);
            if (deltaT > 0)
                height -= deltaT;
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
    rasterBorder(BLUE);
#endif
    /*
     * Sanity check scroll flags
     */
    if (scrolldir & (SCROLL_UP2 | SCROLL_UP) && scrolldir & (SCROLL_DOWN2 | SCROLL_DOWN))
        scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    else
    {
        if ((scrolldir & (SCROLL_UP2 | SCROLL_UP)) == (SCROLL_UP2 | SCROLL_UP))
            scrolldir &= ~(SCROLL_UP2| SCROLL_UP);
        if ((scrolldir & (SCROLL_DOWN2| SCROLL_DOWN)) == (SCROLL_DOWN2| SCROLL_DOWN))
            scrolldir &= ~(SCROLL_DOWN2| SCROLL_DOWN);
    }
    if ((scrolldir & (SCROLL_LEFT2 | SCROLL_RIGHT2)) == (SCROLL_LEFT2 | SCROLL_RIGHT2))
        scrolldir &= ~(SCROLL_LEFT2 | SCROLL_RIGHT2);
    /*
     * Calculate new scroll origin and pre-render edges
     */
    if (scrolldir & SCROLL_LEFT2)
    {
        if (orgS < maxOrgS - 1)
        {
            orgS    = (orgS + 2) & 0xFFFE;
            extS    = orgS + 160;
            orgAddr = (orgAddr + 2) & 0x3FFF;
            /*
             * Fill right edge
             */
            vaddr = (orgAddr + 158) & 0x3FFF;
            tileEdgeV((orgS + 158) & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * widthMap + ((orgS + 158) >> 4));
            spriteIntersectRect(orgS + 158, orgT, 2, 100);
        }
        else
            scrolldir &= ~SCROLL_LEFT2;
    }
    else if (scrolldir & SCROLL_RIGHT2)
    {
        if (orgS > 1)
        {
            orgS    = (orgS - 2) & 0xFFFE;
            extS    = orgS + 160;
            orgAddr = (orgAddr - 2) & 0x3FFF;
            /*
             * Fill in left edge
             */
            vaddr = orgAddr;
            tileEdgeV(orgS & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * widthMap + (orgS >> 4));
            spriteIntersectRect(orgS, orgT, 2, 100);
        }
        else
            scrolldir &= ~SCROLL_RIGHT2;
    }
    if (scrolldir & SCROLL_UP2)
    {
        if (orgT < maxOrgT - 1)
        {
            orgT    = (orgT + 2) & 0xFFFE;
            extT    = orgT + 100;
            orgAddr = (orgAddr + 320) & 0x3FFF;
            /*
             * Fill in botom edge
             */
            hcount = 2;
            haddr  = (orgAddr + 98 * 160) & 0x3FFF;
            tileEdgeH2(orgS & 0x0E, (orgT + 98) & 0x0E, tileMap + ((orgT + 98) >> 4) * widthMap + (orgS >> 4));
            spriteIntersectRect(orgS, orgT + 98, 160, 2);
        }
        else
            scrolldir &= ~SCROLL_UP2;
    }
    else if (scrolldir & SCROLL_DOWN2)
    {
        if (orgT > 1)
        {
            orgT    = (orgT - 2) & 0xFFFE;
            extT    = orgT + 100;
            orgAddr = (orgAddr - 320) & 0x3FFF;
            /*
             * Fill in top edge
             */
            hcount = 2;
            haddr  = orgAddr;
            tileEdgeH2(orgS & 0x0E, orgT & 0x0E, tileMap + (orgT >> 4) * widthMap + (orgS >> 4));
            spriteIntersectRect(orgS, orgT, 160, 2);
        }
        else
            scrolldir &= ~SCROLL_DOWN2;
    }
    else if (scrolldir & SCROLL_UP)
    {
        if (orgT < maxOrgT)
        {
            orgT++;
            extT    = orgT + 100;
            orgAddr = (orgAddr + 160) & 0x3FFF;
            /*
             * Fill in botom edge
             */
            haddr  = (orgAddr + 99 * 160) & 0x3FFF;
            hcount = 1;
            tileEdgeH(orgS & 0x0E, (orgT + 99) & 0x0F, tileMap + ((orgT + 99) >> 4) * widthMap + (orgS >> 4));
            spriteIntersectRect(orgS, orgT + 99, 160, 1);
        }
        else
            scrolldir &= ~SCROLL_UP;
    }
    else if (scrolldir & SCROLL_DOWN)
    {
        if (orgT > 0)
        {
            orgT--;
            extT    = orgT + 100;
            orgAddr = (orgAddr - 160) & 0x3FFF;
            /*
             * Fill in top edge
             */
            haddr  = orgAddr;
            hcount = 1;
            tileEdgeH(orgS & 0x0E, orgT & 0x0F, tileMap + (orgT >> 4) * widthMap + (orgS >> 4));
            spriteIntersectRect(orgS, orgT, 160, 1);
        }
        else
            scrolldir &= ~SCROLL_DOWN;
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
    for (sprite = &spriteTable[0]; sprite < &spriteTable[NUM_SPRITES]; sprite++)
    {
        if (sprite->state)
        {
            if (sprite->state == STATE_ACTIVE)
            {
                spriteIntersectSpriteBuf(sprite);
                /*
                 * State could get updated if another sprite moving underneath
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
                tileBuf(sprite->eraS, sprite->eraT, sprite->eraWidth, sprite->eraHeight, sprite->erasebuf);
                spriteIntersectEraseBuf(sprite);
                tileBuf(sprite->bufS, sprite->bufT, sprite->bufWidth, sprite->bufHeight, sprite->spritebuf);
                spriteIntersectSpriteBuf(sprite);
                spriteBuf(sprite->s - sprite->bufS, 0, sprite->width, sprite->height, sprite->spriteptr, sprite->bufWidth >> 1, sprite->spritebuf);
            }
            else if (sprite->state == STATE_DISABLING)
            {
                tileBuf(sprite->eraS, sprite->eraT, sprite->eraWidth, sprite->eraHeight, sprite->erasebuf);
                spriteIntersectEraseBuf(sprite);
            }
        }
    }
#ifdef PROFILE
    rasterBorder(GREEN);
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
     * Draw updated tiles
     */
    while (tileUpdateCount)
    {
        tileUpdateCount--;
        _cpyBuf(tileUpdateS[tileUpdateCount], tileUpdateT[tileUpdateCount], 16, 16, tileUpdatePtr[tileUpdateCount]);
    }
    /*
     * Draw update sprites
     */
    for (sprite = &spriteTable[0]; sprite < &spriteTable[NUM_SPRITES]; sprite++)
    {
        if (sprite->state)
        {
            if (sprite->state == STATE_MOVING)
            {
                _cpyBuf(sprite->bufS, sprite->bufT, sprite->bufWidth, sprite->bufHeight, sprite->spritebuf);
                sprite->state = STATE_ACTIVE;
            }
            else if (sprite->state == STATE_POSITIONING)
            {
                _cpyBuf(sprite->eraS, sprite->eraT, sprite->eraWidth, sprite->eraHeight, sprite->erasebuf);
                _cpyBuf(sprite->bufS, sprite->bufT, sprite->bufWidth, sprite->bufHeight, sprite->spritebuf);
                sprite->state = STATE_ACTIVE;
            }
            else if (sprite->state == STATE_DISABLING)
            {
                _cpyBuf(sprite->eraS, sprite->eraT, sprite->eraWidth, sprite->eraHeight, sprite->erasebuf);
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

    /*
     * Sanity check scroll flags
     */
    if (scrolldir & (SCROLL_UP2 | SCROLL_UP) && scrolldir & (SCROLL_DOWN2 | SCROLL_DOWN))
        scrolldir &= ~(SCROLL_UP2 | SCROLL_DOWN2 | SCROLL_UP | SCROLL_DOWN);
    else
    {
        if ((scrolldir & (SCROLL_UP2 | SCROLL_UP)) == (SCROLL_UP2 | SCROLL_UP))
            scrolldir &= ~(SCROLL_UP2| SCROLL_UP);
        if ((scrolldir & (SCROLL_DOWN2| SCROLL_DOWN)) == (SCROLL_DOWN2| SCROLL_DOWN))
            scrolldir &= ~(SCROLL_DOWN2| SCROLL_DOWN);
    }
    if ((scrolldir & (SCROLL_LEFT2 | SCROLL_RIGHT2)) == (SCROLL_LEFT2 | SCROLL_RIGHT2))
        scrolldir &= ~(SCROLL_LEFT2 | SCROLL_RIGHT2);
    /*
     * Calculate new scroll origin
     */
    if (scrolldir & SCROLL_LEFT2)
    {
        if (orgS < maxOrgS - 1)
        {
            orgS  = (orgS + 2) & 0xFFFE;
            extS  = orgS + 160;
        }
        else
            scrolldir &= ~SCROLL_LEFT2;
    }
    else if (scrolldir & SCROLL_RIGHT2)
    {
        if (orgS > 1)
        {
            orgS  = (orgS - 2) & 0xFFFE;
            extS  = orgS + 160;
        }
        else
        scrolldir &= ~SCROLL_RIGHT2;
    }
    if (scrolldir & SCROLL_UP2)
    {
        if (orgT < maxOrgT - 1)
        {
            orgT  = (orgT + 2) & 0xFFFE;
            extT  = orgT + 100;
        }
        else
            scrolldir &= ~SCROLL_UP2;
    }
    else if (scrolldir & SCROLL_DOWN2)
    {
        if (orgT > 1)
        {
            orgT  = (orgT - 2) & 0xFFFE;
            extT  = orgT + 100;
        }
        else
            scrolldir &= ~SCROLL_DOWN2;
    }
    else if (scrolldir & SCROLL_UP)
    {
        if (orgT < maxOrgT)
        {
            orgT++;
            extT = orgT + 100;
        }
        else
            scrolldir &= ~SCROLL_UP;
    }
    else if (scrolldir & SCROLL_DOWN)
    {
        if (orgT > 0)
        {
            orgT--;
            extT = orgT + 100;
        }
        else
            scrolldir &= ~SCROLL_DOWN;
    }
    /*
     * Point orgAddr to back buffer
     */
    orgAddr ^= 0x4000;
    /*
     * Draw tiles
     */
    tileScrn(orgS, orgT);
    /*
     * Draw sprites
     */
    for (sprite = &spriteTable[0]; sprite < &spriteTable[NUM_SPRITES]; sprite++)
    {
        if (sprite->state >= STATE_ACTIVE)
        {
            if ((sprite->s < extS)
            && ((sprite->s + sprite->width) > orgS)
            &&  (sprite->t < extT)
            && ((sprite->t + sprite->height) > orgT))
            {
                spriteX      = sprite->s - orgS;
                spriteY      = sprite->t - orgT;
                spriteWidth  = sprite->width;
                spriteHeight = sprite->height;
                spriteImg    = sprite->spriteptr;
                if (spriteX < 0)
                {
                    spriteImg   -= spriteX >> 1;
                    spriteWidth += spriteX;
                    spriteX      = 0;
                }
                if (spriteX + spriteWidth > 160)
                {
                    spriteWidth = 160 - spriteX;
                }
                if (spriteY < 0)
                {
                    spriteImg    -= (spriteY * sprite->width) >> 1;
                    spriteHeight += spriteY;
                    spriteY       = 0;
                }
                if (spriteY + spriteHeight > 100)
                {
                    spriteHeight = 100 - spriteY;
                }
                if (spriteWidth > 1)
                    spriteScrn(spriteX, spriteY, spriteWidth, spriteHeight, sprite->width >> 1, spriteImg);
            }
        }
        else if (sprite->state == STATE_DISABLING)
        {
            free(sprite->spritebuf);
            free(sprite->erasebuf);
            sprite->state     = STATE_INACTIVE;
            sprite->spriteptr = 0L;
            sprite->spritebuf = 0L;
            sprite->erasebuf  = 0L;
        }
    }
    /*
     * Wait until VBlank and update CRTC start address to swap buffers
     */
    while (inp(0x3DA) & 0x08); // Wait until the end of VBlank
    outpw(0x3D4, ((orgAddr >> 1) & 0xFF00) + 12);
    while (!(inp(0x3DA) & 0x08)); // Wait until beginning of VBlank
    /*
     * Return updated origin as 32 bit value
     */
    return ((unsigned long)orgT << 16) | orgS;
}
void viewInit(int adapter, unsigned int s, unsigned int t, unsigned int width, unsigned int height, unsigned char far * far *map)
{
    int i;
    /*
     * Init tile map
     */
    tileMap    = map;
    widthMap   = width;
    spanMap    = widthMap << 2;
    heightMap  = height;
    widthMapS  = width  << 4;
    heightMapT = height << 4;
    maxOrgS    = widthMapS  - 160;
    maxOrgT    = heightMapT - 100;
    orgS       = s & 0xFFFE; // S always even
    orgT       = t;
    extS       = orgS + 160;
    extT       = orgT + 100;
    enableRasterTimer((adapter & 0x7FFF) - 1);
    if (adapter & 0x8000)
    {   // EGA/VGA
        orgAddr = 0x0001; // Draw to front buffer
        viewRefresh = viewRedraw;
        tileScrn(orgS, orgT);
        outpw(0x3D4, 0x0000 + 12);
    }
    else
    {   // CGA
        orgAddr = (orgT * 160 + orgS | 1) & 0x3FFF;
        viewRefresh = viewScroll;
        rasterDisable(); /* Turn off video */
        setStartAddr(orgAddr >> 1);
        tileScrn(orgS, orgT);
        rasterEnable();  /* Turn on video */
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
            spriteTable[i].state     = STATE_INACTIVE;
        }
    }
    disableRasterTimer();
}
