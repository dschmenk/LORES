/*
 * I/O routines to load and save tiles, maps, and sprites.
 */
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include "mapio.h"

int tilesetLoad(char *filename, unsigned char far * *tileset, int sizeoftile)
{
    int count, fd, i, j;
    struct stat buffer;
    unsigned char tile[8*16], far *tileptr;

    count = 0;
    if (stat(filename, &buffer) == 0)
    {
        if (buffer.st_size >= sizeoftile)
        {
            count = buffer.st_size / (8*16);
            *tileset = (unsigned char far *)_fmalloc(count * sizeoftile);
            if (*tileset != NULL)
            {
                tileptr = *tileset;
                fd      = open(filename, O_RDONLY | O_BINARY);
                for (i = 0; i < count; i++)
                {
                    read(fd, (char *)tile, 8*16);
                    for (j = 0; j < 8*16; j++)
                        tileptr[j] = tile[j];
                    tileptr += sizeoftile;
                }
                close(fd);
            }
            else
                count = -1;
        }
    }
    return count;
}
int tilesetSave(char *filename, unsigned char far *tileset, int sizeoftile, int count)
{
    int fd, j;
    unsigned char tile[8*16];

    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY);
    if (fd)
    {
        while (count--)
        {
            for (j = 0; j < 8*16; j++)
                tile[j] = tileset[j];
            write(fd, (char *)tile, 8*16);
            tileset += sizeoftile;
        }
        close(fd);
    }
    return fd ? 0 : -1;
}
unsigned long tilemapLoad(char *filename, unsigned char far *tileset, int sizeoftile, unsigned char far * far * *tilemap)
{
    int fd, width, height, i, j;
    unsigned int index;
    unsigned char far * far *mapptr;

    fd = open(filename, O_RDONLY | O_BINARY);
    if (fd)
    {
        read(fd, (char *)&width,  sizeof(int));
        read(fd, (char *)&height, sizeof(int));
        *tilemap = (unsigned char far * far *)_fmalloc(width * height * sizeof(unsigned char far *));
        if (*tilemap)
        {
            mapptr = *tilemap;
            for (j = 0; j < height; j++)
                for (i = 0; i < width; i++)
                {
                    read(fd, (char *)&index, sizeof(unsigned int));
                    if (index == 0xFFFF)
                        *mapptr++ = NULL;
                    else
                        *mapptr++ = tileset + index * sizeoftile;
                }
        }
        else
            width = height = 0;
        close(fd);
    }
    else
        width = height = 0;
    return ((unsigned long)height << 16) | width;
}
int tilemapSave(char *filename, unsigned char far *tileset, int sizeoftile, unsigned char far * far *tilemap, int width, int height)
{
    int fd, i, j;
    unsigned int index;

    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY);
    if (fd)
    {
        write(fd, (char *)&width,  sizeof(int));
        write(fd, (char *)&height, sizeof(int));
        for (j = 0; j < height; j++)
            for (i = 0; i < width; i++)
            {
                if (*tilemap == NULL)
                {
                    index = 0xFFFF;
                    tilemap++;
                }
                else
                    index = (*tilemap++ - tileset) / sizeoftile;
                write(fd, (char *)&index, sizeof(unsigned int));
            }
        close(fd);
    }
    return fd ? 0 : -1;
}
int spriteLoad(char *filename, unsigned char far * *spritepage, int *width, int *height)
{
    int fd, count, i, j, sizeofsprite;
    unsigned char *sprite, far *spriteptr;

    fd = open(filename, O_RDONLY | O_BINARY);
    if (fd)
    {
        read(fd, (char *)width,  sizeof(int));
        read(fd, (char *)height, sizeof(int));
        read(fd, (char *)&count, sizeof(int));
        sizeofsprite = *width * *height / 2;
        sprite       = malloc(sizeofsprite);
        *spritepage  = (unsigned char far *)_fmalloc(sizeofsprite * count);
        if (*spritepage)
        {
            spriteptr = *spritepage;
            for (i = 0; i < count; i++)
            {
                read(fd, sprite, sizeofsprite);
                for (j = 0; j < sizeofsprite; j++)
                    *spriteptr++ = sprite[j];
            }
        }
        else
            count = 0;
        free(sprite);
        close(fd);
    }
    else
        count = 0;
    return count;
}
int spriteSave(char *filename, unsigned char far *spritepage, int width, int height, int count)
{
    int fd, i, j, sizeofsprite;
    unsigned char *sprite;

    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY);
    if (fd)
    {
        write(fd, (char *)&width,  sizeof(int));
        write(fd, (char *)&height, sizeof(int));
        write(fd, (char *)&count,  sizeof(int));
        sizeofsprite = width * height / 2;
        sprite       = malloc(sizeofsprite);
        for (i = 0; i < count; i++)
        {
            for (j = 0; j < sizeofsprite; j++)
                sprite[j] = *spritepage++;
            write(fd, sprite, sizeofsprite);
        }
        free(sprite);
        close(fd);
    }
    return fd ? 0 : -1;
}
