# Invaders from Outer Space

https://youtu.be/1TRwAGdS_Xs

Earth is being invaded by space aliens. You are Earth's last defense. Based on
the Moon, you must shoot the aliens before they get down to you and destroy
your ship. Otherwise, Earth is lost.

                Controls:
                    LEFT ARROW  - move left
                    RIGHT ARROW - move right
                    SPACEBAR    - fire missile
                    ESCAPE      - quit game

## Implementation Details

Unlike the other testbed programs, "Invaders" only uses scrolling during the
opening and closing scenes. Actual gameplay relies just on sprites to render
the play field. There are a possible 8 active sprites: 1 ship, 1 missile, and 6
aliens. A stock IBM PC cannot update all 8 sprites 60 times a second without
CGA snow or image anomalies so alien movement is interleaved, one alien per
frame. It is hard to recognize that the aliens aren't moving in sync.

The LORES library won't clip sprites to the map boundaries, but it will clip
them to screen boundaries (an implementation performance decision). In order to
get clipped sprites, "Invaders" creates a tile boundary surrounding the
playfield. The view is set inside this boundary so the sprites will be nicely
clipped to the screen edges.

The background was created from a NASA "Earthrise" picture. The slicer.c
program in the MAPEDIT directory is run on a modern computer and will convert
the Portable Net RGB image into either a dithered or a closest match 4 BPP IRGB
version. It will then slice the image in 16x16 pixel tiles, reducing duplicate
tiles, and saving it in a format to be loaded by the LORES library (or further
edited with MAPEDIT).

These are more of the tricks needed to effectively use this library and
graphics mode on a 4.77 MHz IBM PC.
