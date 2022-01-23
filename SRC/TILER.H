#define SCROLL_LEFT     0x01
#define SCROLL_RIGHT    0x02
#define SCROLL_UP       0x04
#define SCROLL_DOWN     0x08

void tileInit(unsigned int s, unsigned int t, unsigned int width, unsigned int height, unsigned char far * far *map);
unsigned long tileScroll(int scrolldir);
void tileBuf(unsigned int s, unsigned int t, int widthBuf, int heightBuf, unsigned char far *buf);
void cpyBuf(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf);
void tileScrn(unsigned int s, unsigned int t);
