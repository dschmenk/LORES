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
	int x, y, spriterows, spritecols, spritecount, spritewidth, spriteheight;
    unsigned char dither, stats, spritenums[6], *sprite;
    float gammafunc;
    char *basename,  pnmname[80], spritename[80];

    gammafunc  = 1.0;
    dither     = 1;
    spriterows = 1;
    spritecols = 1;
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
            spritecols = atoi(argv[2]);
            argc--;
            argv++;
        }
        if (argc > 2 && argv[1][1] == 'r')
        {
            spriterows = atoi(argv[2]);
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
		if (readimage(pnmname, dither, gammafunc) != 0)
        {
            fprintf(stderr, "Unable to read PNM image: %s\n", pnmname);
			return -1;
        }
	}
	else
	{
        fprintf(stderr, "Missing basename\n");
        return -1;
    }
    spritecount  = spriterows * spritecols;
    spritewidth  = mapwidth / spritecols;
    spriteheight = mapheight / spriterows;
    sprite       = malloc((spritewidth + 1) / 2 * spriteheight);
    sprintf(spritename, "%s.spr", basename);
    if ((spritefile = fopen(spritename, "wb")) == NULL)
    {
        fprintf(stderr, "Unable to write sprite file: %s\n", spritename);
        return -1;
    }
    spritenums[0] = spritewidth;
    spritenums[1] = spritewidth >> 8;
    spritenums[2] = spriteheight;
    spritenums[3] = spriteheight >> 8;
    spritenums[4] = spritecount;
    spritenums[5] = spritecount >> 8;
    fwrite(spritenums, 1, 6, spritefile);
    for (y = 0; y < mapheight; y += spriteheight)
        for (x = 0; x < mapwidth; x += spritewidth)
            writesprite(spritefile, pixmap + (y * mapwidth + x) / 2, sprite, spritewidth, mapwidth / 2, spriteheight);
    fclose(spritefile);
}
