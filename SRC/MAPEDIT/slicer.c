#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

FILE *pbmfile;
int pbmwidth, pbmheight, pbmdepth, mapwidth, mapheight;
unsigned char clrgamma[256], *pixmap;
struct _tile
{
    unsigned char tilepix[16 * 16 / 2];
    struct _tile *next;
    int index;
} *hashlist[256];
int hashcount[256];
int tilecount = 0;

/*
 * 8x4 dither matrix (4x4 replicated twice horizontally to fill byte).
 */
#define BRI_PLANE             3
#define RED_PLANE             2
#define GRN_PLANE             1
#define BLU_PLANE             0
static unsigned int ddithmask[16] = // Dark color dither
{
    0x0000,
    0x8000,
    0x8020,
    0x80A0,
    0xA0A0,
    0xA4A0,
    0xA4A1,
    0xA4A5,
    0xA5A5,
    0xADA5,
    0xADA7,
    0xADAF,
    0xAFAF,
    0xEFAF,
    0xEFBF,
    0xEFFF
};
static unsigned int bdithmask[16] = // Bright color dither
{
    0x0000,
    0x8000,
    0x8020,
    0x80A0,
    0xA0A0,
    0xA4A0,
    0xA4A1,
    0xA4A5,
    0xA5A5,
    0xADA5,
    0xADA7,
    0xADAF,
    0xAFAF,
    0xEFAF,
    0xEFBF,
    0xFFFF
};
/*
 * Build a dithered brush and return the closest solid color match to the RGB.
 */
unsigned char brush(unsigned char red, unsigned char grn, unsigned blu, unsigned int *pattern)
{
    unsigned char clr, l, i;

    /*
     * Find MAX(R,G,B)
     */
    if (red >= grn && red >= blu)
        l = red;
    else if (grn >= red && grn >= blu)
        l = grn;
    else // if (blue >= grn && blu >= red)
        l = blu;
    if (l > 127) // 50%-100% brightness
    {
        /*
         * Fill brush based on scaled RGB values (brightest -> 100% -> 0x0F).
         */
        pattern[BRI_PLANE] = bdithmask[(l >> 3) & 0x0F];
        pattern[RED_PLANE] = bdithmask[(red << 4) / (l + 8)];
        pattern[GRN_PLANE] = bdithmask[(grn << 4) / (l + 8)];
        pattern[BLU_PLANE] = bdithmask[(blu << 4) / (l + 8)];
        clr  = 0x08
             | ((red & 0x80) >> 5)
             | ((grn & 0x80) >> 6)
             | ((blu & 0x80) >> 7);
    }
    else // 0%-50% brightness
    {
        /*
         * Fill brush based on dim RGB values.
         */
        if ((l - red) + (l - grn) + (l - blu) < 8)
        {
            /*
             * RGB close to grey.
             */
            if (l > 63) // 25%-50% grey
            {
                /*
                 * Mix light grey and dark grey.
                 */
                pattern[BRI_PLANE] = ~ddithmask[((l - 64) >> 2)];
                pattern[RED_PLANE] =
                pattern[GRN_PLANE] =
                pattern[BLU_PLANE] =  ddithmask[((l - 64) >> 2)];
                clr =  0x07;
            }
            else // 0%-25% grey
            {
                /*
                 * Simple dark grey dither.
                 */
                pattern[BRI_PLANE] = ddithmask[(l >> 2)];
                pattern[RED_PLANE] = 0;
                pattern[GRN_PLANE] = 0;
                pattern[BLU_PLANE] = 0;
                clr = (l > 31) ? 0x08 : 0x00;
            }
        }
        else
        {
            /*
             * Simple 8 color RGB dither.
             */
            pattern[BRI_PLANE] = 0;
            pattern[RED_PLANE] = ddithmask[red >> 3];
            pattern[GRN_PLANE] = ddithmask[grn >> 3];
            pattern[BLU_PLANE] = ddithmask[blu >> 3];
            clr = ((red & 0x40) >> 4)
                | ((grn & 0x40) >> 5)
                | ((blu & 0x40) >> 6);
        }
    }
    return clr;
}
void plot(int x, int y, unsigned char c)
{
    if (x & 1)
        c <<= 4;
    pixmap[(mapwidth * y + x) / 2] |= c;
}
void plotrgb(int x, int y, unsigned char red, unsigned char grn, unsigned char blu)
{
    unsigned int pixbrush[4];
    unsigned char ofst, pix;

    brush(red, grn, blu, pixbrush);
    /*
     * Extract pixel value from IRGB planes.
     */
    ofst = (4 * (y & 3)) + (x & 3);
    pix  = ((pixbrush[BRI_PLANE] >> ofst) & 0x01) << BRI_PLANE;
    pix |= ((pixbrush[RED_PLANE] >> ofst) & 0x01) << RED_PLANE;
    pix |= ((pixbrush[GRN_PLANE] >> ofst) & 0x01) << GRN_PLANE;
    pix |= ((pixbrush[BLU_PLANE] >> ofst) & 0x01) << BLU_PLANE;
    if (x & 1)
        pix <<= 4;
    pixmap[(mapwidth * y + x) / 2] |= pix;
}
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
/*
 * World's dumbest routine to read PGM/PNM files.
 */
int readheader(FILE *fp, int *width, int *height, int *depth)
{
    char pbmheader[80];

    fgets(pbmheader, 80, fp);
    if (strcmp(pbmheader, "P6\n"))
    {
        fprintf(stderr, "PBM header: Missing P6 %s\n", pbmheader);
        return 0;
    }
    do fgets(pbmheader, 80, fp); while (pbmheader[0] == '#');
    switch (sscanf(pbmheader, "%d %d\n", width, height))
    {
        case 0:
            fprintf(stderr, "PBM header: Missing width %s\n", pbmheader);
            return 0;
        case 1:
            if (!fscanf(fp, "%d\n", height))
            {
                fprintf(stderr, "PBM header: Missing height %s\n", pbmheader);
                return 0;
            }
        case 2:
            break;
    }
    return fscanf(fp, "%d\n", depth) == 1;
}
int main(int argc, char **argv)
{
	FILE *pbmfile, *tilefile, *mapfile;
	int pbmwidth, pbmheight, pbmdepth;
	int x, y, tilenum;
    unsigned char dither, stats, r, g, b, mapnums[4];
    float gammafunc;
    unsigned int pixbrush[4];
    char *basename,  pnmname[80], tilename[80], mapname[80];

    for (x = 0; x < 256; x++)
        clrgamma[x] = x;
    dither = 1;
    stats  = 0;
    while (argc > 1 && argv[1][0] == '-')
    {
        if (argc > 2 && argv[1][1] == 'g')
        {
            gammafunc = atof(argv[2]);
            for (x = 0; x < 256; x++)
                clrgamma[x] = pow((float)x/255.0, gammafunc) * 255.0;
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
		if ((pbmfile = fopen(pnmname, "rb")) == NULL)
		{
			fprintf(stderr, "Can't open %s\n", pnmname);
			return -1;
		}
	}
	else
	{
        fprintf(stderr, "Missing basename\n");
        return -1;
    }
	if (!readheader(pbmfile, &pbmwidth, &pbmheight, &pbmdepth))
	{
		fprintf(stderr, "Not a valid PBM file.\n");
		return -1;
	}
    mapwidth  = (pbmwidth  + 15) & 0xFFF0;
    mapheight = (pbmheight + 15) & 0xFFF0;
    pixmap    = malloc(mapwidth * mapheight / 2);
    memset(pixmap, 0, mapwidth * mapheight / 2);
	for (y = 0; y < pbmheight; y++)
		for (x = 0; x < pbmwidth; x++)
        {
            r = clrgamma[getc(pbmfile)];
            g = clrgamma[getc(pbmfile)];
            b = clrgamma[getc(pbmfile)];
            if (dither)
 			    plotrgb(x, y, r, g, b);
            else
                plot(x, y, brush(r, g, b, pixbrush));
        }
    fclose(pbmfile);
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
