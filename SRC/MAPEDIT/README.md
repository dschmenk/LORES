# Tile Set/Map Editor and Sprite Editor
The LORES library provides an API for programming the CGA to implement a virtual tiled map and sprites. Instead of creating assets with graph paper and transcribe them into hexadecimal arrays in source files, tile sets and tiled maps can be created and edited with the MAPEDIT.EXE program, sprites can be created and edited with the SPRITED.EXE program.

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

## Executables

Real-mode MS-DOS programs are located in the [BIN](https://github.com/dschmenk/LORES/tree/main/BIN) directory.
