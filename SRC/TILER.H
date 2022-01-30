#define SCROLL_LEFT2    0x01
#define SCROLL_RIGHT2   0x02
#define SCROLL_UP2      0x04
#define SCROLL_DOWN2    0x08
#define SCROLL_UP       0x10
#define SCROLL_DOWN     0x20

void tileInit(unsigned int s, unsigned int t, unsigned int width, unsigned int height, unsigned char far * far *map);
unsigned long tileScroll(int scrolldir);
void tileBuf(unsigned int s, unsigned int t, int widthBuf, int heightBuf, unsigned char far *buf);
void _cpyBuf(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf);
void _cpyBufSnow(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf);
void tileScrn(unsigned int s, unsigned int t);
#ifdef CGA_SNOW
#define cpyBuf  _cpyBufSnow
#else
#define cpyBuf  _cpyBuf
#endif
