#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "pnmread.c"

struct _tile
{
    unsigned char tilepix[16 * 16 / 2];
    struct _tile *next;
    int index;
} *hashlist[256];
int hashcount[256];
int tilecount = 0;
int addtile(FILE *tilefile, unsigned char *mapptr)
{
    struct _tile newtile, *matchtile;
    unsigned char hash;
    int row, col, tileindex;

    hash = 0;
    for (row = 0; row < 16; row++)
    {
        for (col = 0; col < 8; col++)
        {
            hash += mapptr[col];
            newtile.tilepix[row * 8 + col] = mapptr[col];
        }
        mapptr += mapwidth / 2;
    }
    matchtile = hashlist[hash];
    while (matchtile)
    {
        if (memcmp(newtile.tilepix, matchtile->tilepix, 16 * 8) == 0)
            break;
        matchtile = matchtile->next;
    }
    if (!matchtile)
    {
        newtile.index  = tileindex = tilecount++;
        newtile.next   = hashlist[hash];
        hashlist[hash] = malloc(sizeof(struct _tile));
        memcpy(hashlist[hash], &newtile, sizeof(struct _tile));
        fwrite(newtile.tilepix, 16, 8, tilefile);
        hashcount[hash]++;
    }
    else
        tileindex = matchtile->index;
    return tileindex;
}
int main(int argc, char **argv)
{
	FILE *tilefile, *mapfile;
	int x, y, tilenum;
    unsigned char dither, stats, mapnums[4];
    float gammafunc;
    char *basename,  pnmname[80], tilename[80], mapname[80];

    gammafunc = 1.0;
    dither    = 1;
    stats     = 0;
    while (argc > 1 && argv[1][0] == '-')
    {
        if (argc > 2 && argv[1][1] == 'g')
        {
            gammafunc = atof(argv[2]);
            argc--;
            argv++;
        }
        else if (argv[1][1] == 'n')
        {
            dither = 0;
        }
        else if (argv[1][1] == 's')
        {
            stats = 1;
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
    sprintf(tilename, "%s.set", basename);
    sprintf(mapname, "%s.map", basename);
    for (tilenum = 0; tilenum < 256; tilenum++)
    {
        hashlist[tilenum]  = NULL;
        hashcount[tilenum] = 0;
    }
    tilefile  = fopen(tilename, "wb");
    mapfile   = fopen(mapname, "wb");
    mapnums[0] = (mapwidth / 16);
    mapnums[1] = (mapwidth / 16) >> 8;
    mapnums[2] = (mapheight / 16);
    mapnums[3] = (mapheight / 16) >> 8;
    fwrite(mapnums, 1, 4, mapfile);
    for (y = 0; y < mapheight; y += 16)
        for (x = 0; x < mapwidth; x += 16)
        {
            tilenum = addtile(tilefile, pixmap + (y * mapwidth + x) / 2);
            mapnums[0] = tilenum;
            mapnums[1] = tilenum >> 8;
            fwrite(mapnums, 1, 2, mapfile);
        }
    fclose(tilefile);
    fclose(mapfile);
    if (stats)
    {
        for (tilenum = 0; tilenum < 256; tilenum++)
            printf("Hash count[%d]: %d\n", tilenum ,hashcount[tilenum]);
        printf("Tile count: %d\n", tilecount);
    }
}
