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
#define rasterBorder(c) outp(0x3D9, c)
#define rasterDisable() outp(0x3D8, 0x01)
#define rasterEnable()  outp(0x3D8, 0x09)
void txt80(void);
void gr160(unsigned char fill, unsigned char border);
void line(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, unsigned char color);
void rect(unsigned int x, unsigned int y, int width, int height, unsigned char color);
void text(unsigned int x, unsigned int y, unsigned char color, char *string);
void _plot(unsigned int x, unsigned int y, unsigned char color);
void _hlin(unsigned int x1, unsigned int x2, unsigned int y, unsigned char color);
void _vlin(unsigned int x, unsigned int y1, unsigned int y2, unsigned char color);
void _plotSnow(unsigned int x, unsigned int y, unsigned char color);
void _hlinSnow(unsigned int x1, unsigned int x2, unsigned int y, unsigned char color);
void _vlinSnow(unsigned int x, unsigned int y1, unsigned int y2, unsigned char color);
void _cpyBuf(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf);
void _cpyBufSnow(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf);
#ifdef CGA_SNOW
#define plot    _plotSnow
#define hlin    _hlinSnow
#define vlin    _vlinSnow
#define cpyBuf  _cpyBufSnow
#else
#define plot    _plot
#define hlin    _hlin
#define vlin    _vlin
#define cpyBuf  _cpyBuf
#endif
extern volatile unsigned char rasterTimer;
int enableRasterTimer(int scanline);
int disableRasterTimer(void);
int statusRasterTimer();
void setStartAddr(int addr); // Synchronously set CRTC start address with end of active video
