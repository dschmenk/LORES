# RepelZ

https://www.youtube.com/watch?v=oudSbuUulqI

 Your region has just been invaded by a neighboring country. You must defend
 your homeland by destroying the invading tanks and surface-to-air (SAM)
 launchers with your drone. They will be advancing on roads leading from the
 region boundary (marked in red). Don't go past the red boundary as the enemy
 has full control of the airways and your drone will be immediately destroyed.
 The SAM installations will launch missiles at your drone when in range. The
 faster you fly the drone, the harder it will be for the SAM to hit, but also
 more difficult to fire at the tanks and SAM launchers.

                    Controls:
                        UP ARROW    - speed up
                        DOWN ARROW  - slow down
                        LEFT ARROW  - turn left
                        RIGHT ARROW - turn right
                        SPACEBAR    - fire rocket
                        ESCAPE      - quit game

Good Luck!

(Any similarity to current events may or may not be coincidental)

## How It's Done

Let's look at how to create a full-screen, 16 color, 60 FPS game on a 4.77 MHz
IBM PC with CGA video. First, the LORES library is used to provide the high performance
2D scrolling and sprite functionality. This supports a simple to use API that
does all the heavy lifting, leaving just the game logic to implement.

### Sprite Usage

The 8088 at 4.77 MHz can't push a lot of pixels in the short time allotted during
inactive video to avoid CGA snow and video tearing. The sprites must be of moderate
size to update everything on the screen. Keeping the sprite size from around 8x8
to 12x12 pixels will be necessary to have a few of them being simultaneously active
on the screen. With a 160x100 resolution screen these are bigger than what would
be available on a 320x200 or 640x480 resolution screen, so more appropriate than
might be initially assumed.

### Scrolling Algorithm

One of the limitations of the LORES library is the horizontal two-pixel
scrolling increment. In order to create a consistent appearance during diagonal
scrolling, vertical increments can be set to two-pixel increments as well. By
moving the center around by even pixel coordinates and locking the view to this
sprite, it will be hard to visually recognize that the scrolling is actually
limited to two-pixel amounts. The actual movement is controlled with a combination
of a 16.16 fixed point move rate and a Bresenham algorithm to choose the pixel
coordinates. Other sprites are moved by a simple 16.16 position and increment.

### Mini-Scheduler

The lowly 4.77 MHz 8088 struggles to get much processing done 60 times a second.
Luckily, game logic doesn't all have to be run at 60 Hz, much high level logic
can be run at a slower rate. In order to get all aspects of the game logic to
execute in a timely fashion, different parts of the game logic are scheduled
based on framerate. Game logic is broken up into 4 tasks that are run every
fourth frame. The LORES library provides a frame counter variable that increments
every frame. This can be used to schedule the different tasks such as input
handling, sound generation, enemy AI, and such.

### Efficient Input

There are two methods for keyboard input that are compile time options. The first,
original method uses the C library getch() function which is basically a wrapper
around the BIOS keyboard calls. There are two issues with this approach: high
overhead and hard to control keypress rate. The second method installs a keyboard
IRQ handler to monitor key presses and releases. The key state can be quickly
checked with very little overhead. However, the code uses a feature of the
Borland C++ compiler to implement interrupt routines directly in C. As a result,
this code won't compile under MSC 5.1.
