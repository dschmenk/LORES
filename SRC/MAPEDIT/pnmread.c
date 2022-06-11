int mapwidth, mapheight;
unsigned char *pixmap;
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
/*
 * World's dumbest routine to read PGM/PNM files.
 */
int readimage(char *filename, unsigned char dither, float gammafunc)
{
    FILE *pbmfile;
    char pbmheader[80];
    int pbmwidth, pbmheight, pbmdepth;
    int x, y;
    unsigned char r, g, b, clrgamma[256];
    unsigned int pixbrush[4];

    if ((pbmfile = fopen(filename, "rb")) == NULL)
    {
		fprintf(stderr, "Unable to open %s.\n", filename);
		return -1;
	}
    fgets(pbmheader, 80, pbmfile);
    if (strcmp(pbmheader, "P6\n"))
    {
        fprintf(stderr, "PBM header: Missing P6 %s\n", pbmheader);
        return 0;
    }
    do fgets(pbmheader, 80, pbmfile); while (pbmheader[0] == '#');
    switch (sscanf(pbmheader, "%d %d\n", &pbmwidth, &pbmheight))
    {
        case 0:
            fprintf(stderr, "PBM header: Missing width %s\n", pbmheader);
            return 0;
        case 1:
            if (!fscanf(pbmfile, "%d\n", &pbmheight))
            {
                fprintf(stderr, "PBM header: Missing height %s\n", pbmheader);
                return 0;
            }
        case 2:
            break;
    }
    if (fscanf(pbmfile, "%d\n", &pbmdepth) != 1)
	{
        fprintf(stderr, "PBM header: Missing depth %s\n", pbmheader);
		return -1;
	}
    if (gammafunc != 1.0)
        for (x = 0; x < 256; x++)
            clrgamma[x] = pow((float)x/255.0, gammafunc) * 255.0;
    else
        for (x = 0; x < 256; x++)
            clrgamma[x] = x;
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
    return 0;
}
