# Tile Set/Map Editor, Sprite Editor and Image/Sprite Slicer
The LORES library provides an API for programming the CGA to implement a virtual tiled map and sprites. Instead of creating assets with graph paper and transcribe them into hexadecimal arrays in source files, tile sets and tiled maps can be created and edited with the MAPEDIT.EXE program, sprites can be created and edited with the SPRITED.EXE program. With the sprite rotator, a sprite image can be rotated around the center and saved as a sprite page. 'slicer.c' is another tool that runs on modern OSes that will take a Portable BitMap '.pnm' file and convert it into a format digestible by the LORES MAPIO API. It will reduce the 24 BPP RGB image into a 4BPP IRGB, 16x16 tiled map.

## MAPEDIT

![MAPEDIT](https://github.com/dschmenk/LORES/blob/main/SRC/MAPEDIT/mapedit_000.png "mapedit")

Tileset are loaded and save in `.SET` files. Tilesets are referenced in the virtual tile maps, loaded and saved in `.MAP` files. MAPEDIT will match the base name and load/save both the tileset and tilemap files together. MAPEDIT is run as:

    MAPEDIT [BASENAME]

Without specifying a base name, a blank map and tile set will be created. With a base name, the tile set and tile map will be loaded and displayed for editing. Commands are single key operations:

 |     Key    |      Command       
 |:----------:|--------------------
 |   PgUp     | Move up tile       
 | ShiftUp    | Move up tile       
 |   PgDn     | Move down tile
 | ShiftDown  | Move down tile     
 | ShiftLeft  | Move left tile     
 | ShiftRight | Move right tile    
 |    Up      | Move up pixel       
 |   Down     | Move down pixel     
 |  Right     | Move right pixel    
 |   Left     | Move left pixel    
 |  Space     | Set pixel color    
 |    f       | Fill tile color    
 |    N       | Next pixel color   
 |    n       | Next pixel color   
 |    c       | Select center tile
 |    v       | Paste tile         
 |    x       | eXchange tile      
 |    s       | Select tile list   
 |    p       | Preview tilemap    
 |    t       | new Tile           
 |    K       | Kill (del) tile    
 |    i       | Insert row/col map
 |    d       | Delete row/col map
 |    W       | Write to filename  
 |    w       | Write file         
 |   ESC      | Quit               
 |    q       | Quit               

## SPRITED

![SPRITED](https://github.com/dschmenk/LORES/blob/main/SRC/MAPEDIT/sprited_000.png "sprited")

Sprite pages (multiple same-size sprite images) can be loaded, edited and saved as `.SPR` files. SPRITED is run as:

    SPRITED [BASENAME]

Without specifying a base name, a blank sprite page will be created. Commands are single key operations:

 |     Key    |      Command       
 |:----------:|--------------------
 | ShiftLeft  | Move left sprite   
 | ShiftRight | Move right sprite  
 |    Up      | Move up pixel       
 |   Down     | Move down pixel     
 |  Right     | Move right pixel   
 |   Left     | Move left pixel    
 |  Space     | Set pixel color    
 |    f       | Fill sprite color  
 |    N       | Next pixel color   
 |    n       | Next pixel color   
 |    c       | Select sprite      
 |    v       | Paste sprite       
 |    x       | eXchange sprite    
 |    s       | new Sprite         
 |    K       | Kill (del) sprite    
 |    i       | Insert row/col map
 |    d       | Delete row/col map
 |    W       | Write to filename  
 |    w       | Write file         
 |   ESC      | Quit               
 |    q       | Quit               

## SPROTATE

A sprite image can be rotated around the center by an increment. The first image in a sprite file will be rotated `n` times to complete a full rotation, then re-saved in the sprite page file. Note: any other images in the sprite page will be overwritten. Further editing may be required to clean up any pixels that got rounded poorly.

    SPROTATE [FILENAME] [ROTATE INCREMENTS]

## slicer.c

The 'slicer' tool must be compiled on your modern OS to run. It is very generic C code so should have no problem running on anything remotely modern. It takes a Portable BitMap '.pnm' image file, easily exported by the GIMP (https://www.gimp.org) image processing program and creates the tile set and tile map assets importable using the LORES MAPIO API. It also reduces all duplicate tiles to save space in the tile set.

    ./slicer -g [gamma value] [-n] <basename>

The basename is the name of the '.pnm' file without the extension. The files '\<basename\>.set' and '\<basename\>.map' will be created for importing directly into the game or further editing with MAPEDIT.

## spriter.c

The 'spriter' tool is very similar to the above slicer tool in that it takes a sprite page and slices it up into individual images and creates a LORES MAPIO compatible sprite file. Again, it also takes in a Portable BitMap '.pnm' image easily exported from the GIMP. Programs like [Aseprite](https://www.aseprite.org) can export its sprite sheet to a '.png' file which can be imported into the GIMP and further exported to '.pnm'. Whew, that *is* a lot of file format exporting, but probably easier than creating them by hand with SPRITED.

    ./spriter -g [gamma value] [-n] [-r rows] [-c columns] <basename>

Rows and Columns default to 1 if not specified. Basename is the name of the '.pnm' file without the extension. The file '\<basename\>.spr' will be created for importing directly into the game or further editing with SPRITED.

## Executables

Real-mode MS-DOS programs are located in the [BIN](https://github.com/dschmenk/LORES/tree/main/BIN) directory. 'slicer.c' can be compiled under just about any modern OS:

    cc -o slicer slicer.c
