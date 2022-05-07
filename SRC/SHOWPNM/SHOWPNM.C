#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lores.h"

FILE *pbmfile;
int pbmwidth, pbmheight, pbmdepth;
unsigned char gamma[256];
/*
 * World's dumbest routine to read PGM/PNM files.
 */

int main(int argc, char **argv)
{
	FILE *pbmfile;
	int pbmwidth, pbmheight, pbmdepth;
	int x, y;
    unsigned char dither, r, g, b;
    float gammafunc;
    unsigned int pixbrush[4];

    for (x = 0; x < 256; x++)
        gamma[x] = x;
    dither = 1;
    while (argc > 1 && argv[1][0] == '-')
    {
        if (argc > 2 && argv[1][1] == 'g')
        {
            gammafunc = atof(argv[2]);
            for (x = 0; x < 256; x++)
                gamma[x] = pow((float)x/255.0, gammafunc) * 255.0;
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
		if ((pbmfile = fopen(argv[1], "rb")) == NULL)
		{
			fprintf(stderr, "Can't open %s\n", argv[1]);
			return -1;
		}
	}
	else
		pbmfile = stdin;
	if (fscanf(pbmfile, "P6\n%d\n%d\n%d\n", &pbmwidth, &pbmheight, &pbmdepth) != 3)
	{
		fprintf(stderr, "Not a valid PGM file.\n");
		return -1;
	}
	if (pbmwidth > 160 || pbmheight > 100)
	{
		fprintf(stderr, "PGM too large to  display.n");
		return -1;
	}
 	gr160(BLACK, BLACK);
	for (y = 0; y < pbmheight; y++)
		for (x = 0; x < pbmwidth; x++)
        {
            r = gamma[getc(pbmfile)];
            g = gamma[getc(pbmfile)];
            b = gamma[getc(pbmfile)];
            if (dither)
 			    plotrgb(x, y, r, g, b);
            else
                plot(x, y, brush(r, g, b, pixbrush));
        }
	getchar();
	txt80();
}
