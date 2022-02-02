#define MOVE_LEFT2      0x01
#define MOVE_RIGHT2     0x02
#define MOVE_UP2        0x04
#define MOVE_DOWN2      0x08
#define MOVE_LEFT       0x10
#define MOVE_RIGHT      0x20
#define MOVE_UP         0x40
#define MOVE_DOWN       0x80
#define NUM_SPRITES 16

void spriteBuf(int x, int y, int width, int height, unsigned char far *sprite, int span, unsigned char far *buf);
void spriteEnable(int index, unsigned int s, unsigned int t, int width, int height, unsigned char far *sprite);
void spriteDisable(int index);
unsigned long spriteMove(int index, int movedir);
void spritePosition(int index, unsigned int s, unsigned int t);
void spriteUpdate(int index, unsigned char far *image);
void spriteRefresh(void);
