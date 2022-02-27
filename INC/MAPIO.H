/*
 * I/O routines to load and save tiles, maps, and sprites.
 */
int tilesetLoad(char *filename, unsigned char far * *tileset, int sizeoftile);
int tilesetSave(char *filename, unsigned char far *tileset, int sizeoftile, int count);
unsigned long tilemapLoad(char *filename, unsigned char far *tileset, int sizeoftile, unsigned char far * far * *tilemap);
int tilemapSave(char *filename, unsigned char far *tileset, int sizeoftile, unsigned char far * far *tilemap, int width, int height);
int spriteLoad(char *filename, unsigned char far * *spritepage, int *width, int *height);
int spriteSave(char *filename, unsigned char far *spritepage, int width, int height, int count);
