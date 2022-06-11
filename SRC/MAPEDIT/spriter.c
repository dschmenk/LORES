#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "pnmread.c"

int writesprite(FILE *spritefile, unsigned char *pixptr, unsigned char *spriteptr, int width, int span, int height)
{
    int row, col;

    width /= 2;
    for (row = 0; row < height; row++)
    {
        for (col = 0; col < width; col++)
            spriteptr[row * width + col] = pixptr[col];
        pixptr += span;
    }
    return fwrite(spriteptr, width, height, spritefile);
}
int main(int argc, char **argv)
{
	FILE *spritefile;
	int x, y, spritecount, spritewidth;
    unsigned char dither, stats, spritenums[6], *sprite;
    float gammafunc;
    char *basename,  pnmname[80], spritename[80];

    gammafunc   = 1.0;
    dither      = 1;
    spritecount = 1;
    while (argc > 1 && argv[1][0] == '-')
    {
        if (argc > 2 && argv[1][1] == 'g')
        {
            gammafunc = atof(argv[2]);
            argc--;
            argv++;
        }
        if (argc > 2 && argv[1][1] == 'c')
        {
            spritecount = atoi(argv[2]);
            argc--;
            argv++;
        }
        else if (argv[1][1] == 'n')
        {
            dither = 0;
        }
        argc--;
        argv++;
    }
	if (argc > 1)
	{
        basename = argv[1];
        sprintf(pnmname, "%s.pnm", basename);
		if (!readimage(pnmname, dither, gammafunc))
			return -1;
	}
	else
	{
        fprintf(stderr, "Missing basename\n");
        return -1;
    }
    spritewidth = mapwidth / spritecount;
    sprite = malloc((spritewidth + 1) / 2 * mapheight);
    sprintf(spritename, "%s.spr", basename);
    spritefile  = fopen(spritename, "wb");
    spritenums[0] = (spritewidth / 16);
    spritenums[1] = (spritewidth / 16) >> 8;
    spritenums[2] = (mapheight   / 16);
    spritenums[3] = (mapheight   / 16) >> 8;
    spritenums[4] = (spritecount / 16);
    spritenums[5] = (spritecount / 16) >> 8;
    fwrite(spritenums, 1, 6, spritefile);
    for (x = 0; x < mapwidth; x += spritewidth)
        writesprite(spritefile, pixmap + x / 2, sprite, spritewidth, mapwidth / 2, mapheight);
    fclose(spritefile);
}
