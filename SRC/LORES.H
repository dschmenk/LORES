#define BLACK           0x00
#define DARKBLUE        0x01
#define DARKGREEN       0x02
#define DARKRED         0x04
#define BROWN           (DARKGREEN|DARKRED)
#define DARKCYAN        (DARKBLUE|DARKGREEN)
#define MAROON          (DARKBLUE|DARKRED)
#define GREY            (DARKBLUE|DARKGREEN|DARKRED)
#define DARKGREY        0x08
#define BLUE            0x09
#define GREEN           0x0A
#define RED             0x0C
#define YELLOW          (GREEN|RED)
#define CYAN            (BLUE|GREEN)
#define MAGENTA         (BLUE|RED)
#define WHITE           (BLUE|GREEN|RED)
void txt80(void);
void gr160(unsigned char color);
void plot(unsigned int x, unsigned int y, unsigned char color);
void hlin(unsigned int x1, unsigned int x2, unsigned int y, unsigned char color);
void vlin(unsigned int x, unsigned int y1, unsigned int y2, unsigned char color);
void line(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned char color);
int rect(unsigned int x, unsigned int y, int width, int height, unsigned char color);
void _cpyBuf(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf);
void _cpyBufSnow(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf);
#ifdef CGA_SNOW
#define cpyBuf  _cpyBufSnow
#else
#define cpyBuf  _cpyBuf
#endif
