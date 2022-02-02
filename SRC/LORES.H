void txt80(void);
void gr160(unsigned char color);
void plot(unsigned int x, unsigned int y, unsigned char color);
int rect(unsigned int x, unsigned int y, int width, int height, unsigned char color);
void _cpyBuf(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf);
void _cpyBufSnow(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf);
#ifdef CGA_SNOW
#define cpyBuf  _cpyBufSnow
#else
#define cpyBuf  _cpyBuf
#endif
