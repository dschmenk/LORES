/*
 * External raster timer
 */
extern volatile unsigned int frameCount;
/*
 * Tiler defines and routines
 */
void tileUpdate(unsigned i, unsigned j, unsigned char far *tileNew);
void tileBuf(unsigned int s, unsigned int t, int widthBuf, int heightBuf, unsigned char far *buf);
void tileScrn(unsigned int s, unsigned int t);
/*
 * Sprite defines and routines
 */
#define NUM_SPRITES 4
void spriteBuf(int x, int y, int width, int height, unsigned char far *sprite, int span, unsigned char far *buf);
void spriteEnable(int index, unsigned int s, unsigned int t, int width, int height, unsigned char far *sprite);
void spriteDisable(int index);
//void spriteImage(int index, unsigned char far *imageNew);
void spriteUpdate(int index, unsigned char far *imageNew);
unsigned long spritePosition(int index, unsigned int s, unsigned int t);
/*
 * Tiler and sprite updater
 */
#define SCROLL_LEFT2    0x01
#define SCROLL_RIGHT2   0x02
#define SCROLL_UP2      0x04
#define SCROLL_DOWN2    0x08
#define SCROLL_UP       0x40
#define SCROLL_DOWN     0x80
void viewInit(unsigned int s, unsigned int t, unsigned int width, unsigned int height, unsigned char far * far *map);
void viewExit(void);
unsigned long viewRefresh(int scrolldir);
