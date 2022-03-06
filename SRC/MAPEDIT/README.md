# Tile Set/Map Editor and Sprite Editor
The LORES library provides an API for programming the CGA to implement a virtual tiled map and sprites. Instead of creating assets with graph paper and transcribe them into hexadecimal arrays in source files, tile sets and tiled maps can be created and edited with the MAPEDIT.EXE program, sprites can be created and edited with the SPRITED.EXE program.

## MAPEDIT
Tileset are loaded and save in `.SET` files. Tilesets are referenced in the virtual tile maps, loaded and saved in `.MAP` files. MAPEDIT will match the base name and load/save both the tileset and tilemap files together. MAPEDIT is run as:

    MAPEDIT [BASENAME]

Without specifying a base name, a blank map and tile set will be created. With a base name, the tile set and tile map will be loaded and displayed for editing. Commands are single key operations:

 |     Key    |      Command       |
 -----------------------------------
 |   PgUp     | Move tile up       |
 | ShiftUp    | Move tile up       |
 |   PgDn     | Move tile down     |
 | ShiftDown  | Move tile down     |
 | ShiftLeft  | Move tile left     |
 | ShiftRight | Move tile right    |
 |    Up      | Move pixel up      |
 |   Down     | Move pixel down    |
 |  Right     | Move pixel right   |
 |   Left     | Move pixel left    |
 |  Space     | Set pixel color    |
 |    f       | Fill tile color    |
 |    N       | Next pixel color   |
 |    n       | Next pixel color   |
 |    c       | Select center tile |
 |    v       | Paste tile         |
 |    x       | eXchange tile      |
 |    s       | Select tile list   |
 |    p       | Preview tilemap    |
 |    t       | new Tile           |
 |    K       | Kill (del) tile    |
 |    i       | Insert row/col map |
 |    d       | Delete row/col map |
 |    W       | Write to filename  |
 |    w       | Write file         |
 |   ESC      | Quit               |
 |    q       | Quit               |
 ---------------------------------

## SPRITED
Sprite pages (multiple same-size sprite images) can be loaded, edited and saved as `.SPR` files. SPRITED is run as:

    SPRITED [BASENAME]

Without specifying a base name, a blank sprite page will be created. Commands are single key operations:


 |     Key    |      Command       |
 -----------------------------------
 | ShiftLeft  | Move sprite left   |
 | ShiftRight | Move sprite right  |
 |    Up      | Move pixel up      |
 |   Down     | Move pixel down    |
 |  Right     | Move pixel right   |
 |   Left     | Move pixel left    |
 |  Space     | Set pixel color    |
 |    f       | Fill sprite color  |
 |    N       | Next pixel color   |
 |    n       | Next pixel color   |
 |    c       | Select sprite      |
 |    v       | Paste sprite       |
 |    x       | eXchange sprite    |
 |    s       | new Sprite         |
 |    K       | Kill (del) tile    |
 |    i       | Insert row/col map |
 |    d       | Delete row/col map |
 |    W       | Write to filename  |
 |    w       | Write file         |
 |   ESC      | Quit               |
 |    q       | Quit               |
 -----------------------------------
