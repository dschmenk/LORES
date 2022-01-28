_TEXT   SEGMENT  WORD PUBLIC 'CODE'
_TEXT   ENDS
_DATA   SEGMENT  WORD PUBLIC 'DATA'
_DATA   ENDS
CONST   SEGMENT  WORD PUBLIC 'CONST'
CONST   ENDS
_BSS    SEGMENT  WORD PUBLIC 'BSS'
_BSS    ENDS
DGROUP  GROUP    CONST, _BSS, _DATA
        ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP

EXTRN   _scanline:BYTE
EXTRN   _edgeH:BYTE
EXTRN   _edgeV:BYTE
EXTRN   _tileMap:BYTE
EXTRN   _spanMap:WORD

_TEXT   SEGMENT

        ASSUME  DS: NOTHING

_tileMem        PROC NEAR
;   s         = bp+4
;   t         = bp+6
;   widthBuf  = bp+8
;   heightBuf = bp+10
;   buf       = bp+12, bp+14
;   x         = bp-2
;   y         = bp-4
;   ss        = bp-6
;   tilerow   = bp-10, bp-8
;   tile      = bp-14, bp-12
;   span      = bp-16
;   width     = bp-18
;   height    = bp-20
; es:di => buf
; ds:si => tile
; bx => width
; dx => height
        les     di, DWORD PTR [bp+12]   ; buf
;    tile   += t * 8 + (s >> 1);
        mov     si, WORD PTR [bp+6]     ; t
        shl     si, 1
        shl     si, 1
        shl     si, 1
        mov     cx, WORD PTR [bp-6]     ;ss
        shr     cx, 1
        add     si, cx
        add     si, WORD PTR [bp-14]    ; OFFSET tile
        mov     ds, WORD PTR [bp-12]    ; SEG tile
        mov     cx, WORD PTR [bp+8]     ; widthBuf
        shr     cx, 1
        mov     bx, WORD PTR [bp-18]    ; width
        shr     bx, 1
        sub     cx, bx
        mov     WORD PTR [bp-16], cx    ; span
        mov     dx, WORD PTR [bp-20]    ; height
tilemem:
        mov     cx, bx
        shr     cx, 1
    rep movsw
        jae     SHORT tmeven            ; jump carry clear
        movsb
tmeven:
        add     di, WORD PTR [bp-16]    ; span
        add     si, 8                   ; tile
        sub     si, bx
        dec     dx                      ; height
        jnz     tilemem
        ret
_tileMem        ENDP

;
; Tile into memory buffer
;
_tileBufRow     PROC NEAR
;   s         = bp+4
;   t         = bp+6
;   widthBuf  = bp+8
;   heightBuf = bp+10
;   buf       = bp+12, bp+14
;   x         = bp-2
;   y         = bp-4
;   ss        = bp-6
;   tilerow   = bp-10, bp-8
;   tile      = bp-14, bp-12
;   span      = bp-16
;   width     = bp-18
;   height    = bp-20
        mov     ax, WORD PTR [bp+4]     ; s
        mov     WORD PTR [bp-6], ax     ; ss
; x = 16 - s; // x is the width of the first tile and start of the second tile column
        sub     ax, 16
        neg     ax
        mov     WORD PTR [bp-2], ax     ; x
; if (x >= widthBuf)
        cmp     ax, WORD PTR [bp+8]     ; widthBuf
        jl      SHORT tilewiderow
;
; Only one tile wide
;
        mov     ax, WORD PTR [bp+8]     ; widthBuf
        mov     WORD PTR [bp-18], ax    ; width = widthBuf
        lds     si, DWORD PTR [bp-10]   ; tilerow
        lodsw
        mov     WORD PTR [bp-14], ax    ; OFFSET tile
        lodsw
        mov     WORD PTR [bp-12], ax    ; SEG tile
        jmp     _tileMem
;
; Two or more tiles wide
;
tilewiderow:
; tileMem(s, t, x, height, *tile++, widthBuf >> 1, buf);
        mov     WORD PTR [bp-18], ax    ; width = x
        lds     si, DWORD PTR [bp-10]   ; tilerow
        lodsw
        mov     WORD PTR [bp-14], ax    ; OFFSET tile
        lodsw
        mov     WORD PTR [bp-12], ax    ; SEG tile
        call    _tileMem
        add     WORD PTR [bp-10], 4     ; tilerow++
        mov     BYTE PTR [bp-6], 0      ; ss
; buf += x >> 1;
        mov     ax, WORD PTR [bp-2]     ; x
        shr     ax, 1
        add     WORD PTR [bp+12], ax    ; OFFSET buf
        jmp     SHORT @F
tilemidrow:
; tileMem(0, t, 16, height, *tile++, widthBuf >> 1, buf);
        mov     WORD PTR [bp-18], 16    ; width = 16
        lds     si, DWORD PTR [bp-10]   ; tilerow
        lodsw
        mov     WORD PTR [bp-14], ax    ; OFFSET tile
        lodsw
        mov     WORD PTR [bp-12], ax    ; SEG tile
        call    _tileMem
; tile++;
        add     WORD PTR [bp-10], 4     ; OFFSET tilerow
; buf += 8;
        add     WORD PTR [bp+12], 8     ; OFFSET buf
; x   += 16;
        add     WORD PTR [bp-2], 16     ; x
; while (x < (widthBuf - 16))
@@:     mov     ax, WORD PTR [bp+8]     ; widthBuf
        sub     ax, 16
        cmp     ax, WORD PTR [bp-2]     ; x
        jg      tilemidrow
; tileMem(0, t, widthBuf - x, height, *tile, widthBuf >> 1, buf);
        mov     ax, WORD PTR [bp+8]     ; widthBuf
        sub     ax, WORD PTR [bp-2]     ; x
        mov     WORD PTR [bp-18], ax    ; width = widthBuf - x
        lds     si, DWORD PTR [bp-10]   ; tilerow
        lodsw
        mov     WORD PTR [bp-14], ax    ; OFFSET tile
        lodsw
        mov     WORD PTR [bp-12], ax    ; SEG tile
        jmp     _tileMem
_tileBufRow    ENDP

        ASSUME  DS: DGROUP

;void tileBuf(unsigned int s, unsigned int t, int widthBuf, int heightBuf, unsigned char far *buf)
        PUBLIC  _tileBuf
_tileBuf    PROC NEAR
        push    bp
        mov     bp, sp
        sub     sp, 24
        push    di
        push    si
;   s         = bp+4
;   t         = bp+6
;   widthBuf  = bp+8
;   heightBuf = bp+10
;   buf       = bp+12, bp+14
;   x         = bp-2
;   y         = bp-4
;   ss        = bp-6
;   tilerow   = bp-10, bp-8
;   tile      = bp-14, bp-12
;   span      = bp-16
;   width     = bp-18
;   height    = bp-20
;
; unsigned char far * far *tilerow;
; tilerow = tileMap + (t >> 4) * spanMap + (s >> 4);
        mov     ax, WORD PTR [bp+6]         ; t
        mov     cl, 4
        shr     ax, cl
        mul     WORD PTR _spanMap
        mov     dx, WORD PTR [bp+4]         ; s
        and     dl, 0F0h
        shr     dx, 1
        shr     dx, 1
        add     ax, dx
        add     ax, WORD PTR _tileMap
        mov     dx, WORD PTR _tileMap+2
        mov     WORD PTR [bp-10], ax        ; OFFSET tilerow
        mov     WORD PTR [bp-8], dx         ; SEG tilerow
; s &= 0x0F;
        and     WORD PTR [bp+4], 0Fh        ; s
; t &= 0x0F;
        and     WORD PTR [bp+6], 0Fh        ; t
; y  = 16 - t; // y is the height of the first tile and start of second tile row
        mov     ax, 16
        sub     ax, WORD PTR [bp+6]         ; t
        mov     WORD PTR [bp-4], ax         ; y
; if (y >= heightBuf)
        cmp     ax, WORD PTR [bp+10]        ; heightBuf
        jl      SHORT tiletallrows
;
; Only one tile tall
;
; tileBufRow(s, t, heightBuf, tilerow, widthBuf, buf);
        mov     ax, WORD PTR [bp+10]        ; height = heightBuf
        jmp     SHORT tilebuflast
;
; Two or more tiles tall
;
tiletallrows:
; tileBufRow(s, t, y, tilerow, widthBuf, buf);
        mov     WORD PTR [bp-20], ax        ; height = y
        push    WORD PTR [bp+12]            ; OFFSET buf
        push    WORD PTR [bp-10]            ; OFFSET tilerow
        push    ds
        call    _tileBufRow
        pop     ds
; tilerow += spanMap;
        pop     ax
        add     ax, WORD PTR _spanMap
        mov     WORD PTR [bp-10], ax        ; OFFSET tilerow
; buf     += (widthBuf * y) >> 1;
        mov     ax, WORD PTR [bp+8]         ; widthBuf
        mul     WORD PTR [bp-20]            ; height
        shr     ax, 1
        pop     bx                          ; OFFSET buf
        add     ax, bx
        mov     WORD PTR [bp+12], ax        ; OFFSET buf
        mov     BYTE PTR [bp+6], 0          ; t
        jmp     SHORT @F
tilemidrows:
; tileBufRow(s, 0, 16, tilerow, widthBuf, buf);
        mov     WORD PTR [bp-20], 16        ; height = 16
        push    WORD PTR [bp+12]            ; OFFSET buf
        push    WORD PTR [bp-10]            ; OFFSET tilerow
        push    ds
        call    _tileBufRow
        pop     ds
; tilerow += spanMap;
        pop     ax
        add     ax, WORD PTR _spanMap
        mov     WORD PTR [bp-10], ax        ; OFFSET tilerow
; buf     += widthBuf * 8;
        mov     ax, WORD PTR [bp+8]         ; widthBuf
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        pop     bx                          ; OFFSET buf
        add     ax, bx
        mov     WORD PTR [bp+12], ax        ; OFFSET buf
; y       += 16;
        add     WORD PTR [bp-4], 16         ; y
; while (y < (heightBuf - 16))
@@:     mov     ax, WORD PTR [bp+10]        ; heightBuf
        sub     ax, 16
        cmp     ax, WORD PTR [bp-4]         ; y
        jg      tilemidrows
; tileBufRow(s, 0, heightBuf - y, tilerow, widthBuf, buf);
        mov     ax, WORD PTR [bp+10]        ; heightBuf
        sub     ax, WORD PTR [bp-4]         ; y
tilebuflast:
        mov     WORD PTR [bp-20], ax        ; height = heightBuf - y
        call    _tileBufRow
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_tileBuf        ENDP

        PUBLIC  _tileEdgeH
_tileEdgeH      PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
;   s    = 4
;   t    = 6
;   tile = 8
;   es:di => &edgeH[x]
;   dx:bx => tile
;   ds:si => *tile
        push    ds
        mov     ax, ds
        mov     es, ax
        ASSUME  DS: NOTHING, ES: DGROUP
        mov     di, OFFSET DGROUP:_edgeH
        mov     bx, WORD PTR [bp+8]         ; OFFSET tile
        mov     dx, WORD PTR [bp+10]        ; SEG tile
; tileMemH(0, s, t, 16 - s, *tile++);
        mov     ds, dx
        lds     si, [bx]                    ; *tile
        mov     cx, 8
        mov     ax, WORD PTR [bp+4]         ; s
        shr     ax, 1                       ; s >> 1
        sub     cx, ax                      ; (16 - s) >> 1
        add     si, ax                      ; s
        ;    tile += (t << 3) + (s >> 1);
        mov     ax, WORD PTR [bp+6]
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        mov     WORD PTR [bp+6], ax         ; t => t << 3
        add     si, ax
        shr     cx, 1
    rep movsw
        jae     SHORT @F
        movsb
@@:     add     bx, 4                       ; tile++
; for (x = 16 - s; x < 160 - 16; x += 16)
tileedgeh:
; tileMemH(x, 0, t, 16, *tile++);
        mov     ds, dx
        lds     si, [bx]                    ; *tile
        add     si, WORD PTR [bp+6]         ; t << 3
        mov     cx, 8/2
    rep movsw
        add     bx, 4                       ; tile++;
        cmp     di, OFFSET DGROUP:_edgeH + (160 - 16) / 2
        jb      tileedgeh
; tileMemH(x, 0, t, 160 - x, *tile++);
        mov     ds, dx
        lds     si, [bx]                    ; *tile
        mov     cx, OFFSET DGROUP:_edgeH + 160 / 2
        sub     cx, di                      ; (160 - x) >> 1
        add     si, WORD PTR [bp+6]         ; t
        shr     cx, 1
    rep movsw
        jae     SHORT @F
        movsb
@@:     pop     ds
        ASSUME  DS: DGROUP, ES: NOTHING
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_tileEdgeH      ENDP

        PUBLIC  _tileEdgeH2
_tileEdgeH2     PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
;   s    = bp+4
;   t    = bp+6
;   tile = bp+8
;   x    = bp-2
;   es:di => &edgeH[x]
;   dx:bx => tile
;   ds:si => *tile
        push    ds
        mov     ax, ds
        mov     es, ax
        ASSUME  DS: NOTHING, ES: DGROUP
        mov     di, OFFSET DGROUP:_edgeH
        mov     bx, WORD PTR [bp+8]         ; OFFSET tile
        mov     dx, WORD PTR [bp+10]        ; SEG tile
; tileMemH2(0, s, t, 16 - s, *tileptr++);
        mov     ds, dx
        lds     si, [bx]                    ; *tile
        mov     cx, 8
        mov     ax, WORD PTR [bp+4]         ; s
        shr     ax, 1                       ; s >> 1
        sub     cx, ax                      ; (16 - s) >> 1
        add     si, ax                      ; s
        ;    tile += (t << 3) + (s >> 1);
        mov     ax, WORD PTR [bp+6]
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        mov     WORD PTR [bp+6], ax         ; t => t << 3
        add     si, ax
;
; Even row
;
        push    cx
        shr     cx, 1
    rep movsw
        jae     SHORT @F
        movsb
@@:     pop     cx
;
; Odd row
;
        mov     ax, WORD PTR [bp+4]         ; s
        shr     ax, 1                       ; s >> 1
        add     si, ax                      ; s
        sub     di, cx
        add     di, 80
        shr     cx, 1
    rep movsw
        jae     SHORT @F
        movsb
@@:     sub     di, 80
        add     bx, 4                       ; tile++
; for (x = 16 - s; x < 160 - 16; x += 16)
tileedgeh2:
; tileMemH2(x, 0, t, 16, *tileptr++);
        mov     ds, dx
        lds     si, [bx]                    ; *tile
        add     si, WORD PTR [bp+6]         ; t << 3
;
; Even row
;
        mov     cx, 8/2
    rep movsw
;
; Odd row
;
        add     di, 80 - 16/2
        mov     cx, 8/2
    rep movsw
        sub     di, 80
        add     bx, 4                       ; tile++;
        cmp     di, OFFSET DGROUP:_edgeH + (160 - 16) / 2
        jb      tileedgeh2
; tileMemH2(x, 0, t, 160 - x, *tileptr++);
        mov     ds, dx
        lds     si, [bx]                    ; *tile
        mov     cx, OFFSET DGROUP:_edgeH + 160 / 2
        sub     cx, di                      ; (160 * 2 - x) >> 1
        add     si, WORD PTR [bp+6]         ; t
;
; Even row
;
        mov     bx, cx                      ; done with bx
        shr     cx, 1
    rep movsw
        jae     SHORT @F
        movsb
@@:     mov     cx, bx
;
; Odd row
;
        sub     di, cx
        add     di, 80
        shr     cx, 1
    rep movsw
        jae     SHORT @F
        movsb
@@:     pop     ds
        ASSUME  DS: DGROUP, ES: NOTHING
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_tileEdgeH2      ENDP

        PUBLIC  _tileEdgeV

; void tileEdgeV(unsigned int s, unsigned int t, unsigned char far * far *tile)
_tileEdgeV      PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
;   s = bp+4
;   t = bp+6
;   tileptr = bp+8
;   es:di => &edgeV[y]
;   dx:bx => tile
;   ds:si => *tile
        push    ds
        mov     ax, ds
        mov     es, ax
        ASSUME  DS: NOTHING, ES: DGROUP
        mov     di, OFFSET DGROUP:_edgeV
        mov     bx, WORD PTR [bp+8]         ; OFFSET tile
        mov     dx, WORD PTR [bp+10]        ; SEG tile
; tileMemV(0, s, t, 16 - t, *tileptr);
        mov     ds, dx
        lds     si, [bx]                    ; *tile
        mov     cx, 16
        mov     ax, WORD PTR [bp+6]         ; t
        sub     cx, ax                      ; 16 - t
; tile += (t << 3) + (s >> 1);
        shr     WORD PTR [bp+4], 1          ; s => s >> 1
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        add     si, WORD PTR [bp+4]         ; s
        add     si, ax
@@:     movsb
        add     si, 7
        loop    @B
; tile += spanMap;
        add     bx, DGROUP: es:_spanMap     ; tile
; for (y = 16 - t; y < 100 - 16; y += 16)
tileedgev:
; tileMemV(y, s, 0, 16, *tileptr);
        mov     ds, dx
        lds     si, [bx]                    ; *tile
        add     si, WORD PTR [bp+4]         ; s
        REPT    16
        movsb
        add     si, 7
        ENDM
; tileptr += spanMap;
        add     bx, DGROUP: es:_spanMap     ; tile
        cmp     di, OFFSET DGROUP:_edgeV + 100 - 16
        jb      tileedgev
; tileMemV(y, s, 0, 100 - y, *tileptr);
        mov     ds, dx
        lds     si, [bx]                    ; *tile
        mov     cx, OFFSET DGROUP:_edgeV + 100
        sub     cx, di                      ; 100 - y
        add     si, WORD PTR [bp+4]         ; s
@@:     movsb
        add     si, 7
        loop    @B
        pop     ds
        ASSUME  DS: DGROUP, ES: NOTHING
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_tileEdgeV    ENDP

PUBLIC    _spriteBuf

;void spriteBuf(int x, int y, int width, int height, unsigned char far *sprite, int span, unsigned char far *buf)
_spriteBuf      PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
        ASSUME  DS: NOTHING
;    x      = bp+4
;    y      = bp+6
;    width  = bp+8
;    height = bp+10
;    sprite = bp+12, bp+14
;    span   = bp+16
;    buf    = bp+18, bp+20
; buf += y * span + (x >> 1);
        mov     ax, WORD PTR [bp+6]     ; y
        mov     cx, WORD PTR [bp+4]     ; x
        shr     cx, 1
        imul    WORD PTR [bp+16]        ; span
        add     ax, cx
        add     WORD PTR [bp+18], ax    ; OFFSET buf
; width >>= 1;
        shr     WORD PTR [bp+8], 1      ; width
; span   -= width;
        mov     ax, WORD PTR [bp+8]     ; width
        sub     WORD PTR [bp+16], ax    ; span
        lds     si, DWORD PTR [bp+12]   ; sprite
        les     di, DWORD PTR [bp+18]   ; buf
; if (x & 1)
        test    BYTE PTR [bp+4], 1      ; x
        je      evenx
; while (height--)
        jmp     SHORT $L20007
$L20005:
; for (w = 0; w < width; w++)
;        mov     WORD PTR [bp-4], 0      ; w
        cmp     WORD PTR [bp+8], 0      ; width
        jle     $FB388
        mov     ax, WORD PTR [bp+8]     ; width
        mov     WORD PTR [bp-6], ax
;        add     WORD PTR [bp-4], ax     ; w
; pix = *spriteptr & 0x0F;
        mov     dl, BYTE PTR [si]
        and     dl, 0Fh
$L20003:
; if (pix != 0x08)
        cmp     dl, 8                   ; Transparent?
        je      $I389
; *buf = (*buf & 0x0F) | (pix << 4);
        mov     al, BYTE PTR es:[di]
        and     al, 0Fh
        mov     bx, dx
        mov     cl, 4
        shl     bl, cl
        or      al, bl
        mov     BYTE PTR es:[di], al
$I389:
; buf++;
        inc     si
; pix = *sprite++ & 0xF0;
        mov     dl, BYTE PTR [si]
        and     dl, 0F0h
        inc     di
; if (pix != 0x80)
        cmp     dl, 128
        je      $I390
; *buf = (*buf & 0xF0) | (pix >> 4);
        mov     ax, dx
        sub     ah, ah
        mov     cl, 4
        shr     ax, cl
        mov     cl, BYTE PTR es:[di]
        and     cl, 0F0h
        or      al, cl
        mov     BYTE PTR es:[di], al
$I390:
        dec     WORD PTR [bp-6]
        jne     $L20003
        mov     WORD PTR [bp+12], di    ; OFFSET sprite
;        mov     BYTE PTR [bp-2], dl     ; pix
$FB388:
        add     di, WORD PTR [bp+16]    ; span
$L20007:
        mov     ax, WORD PTR [bp+10]    ; height
        dec     WORD PTR [bp+10]        ; height
        or      ax, ax
        jne     $L20005
        jmp     SHORT $L20021
; else
evenx:
        mov     dx, WORD PTR [bp+10]    ; height
$L20010:
        mov     cx, WORD PTR [bp+8]     ; width
$L20008:
        lodsb
        cmp     al, 088h                ; Both transparent?
        je      nextpix
        mov     bl, al
        mov     bh, al
        and     bx, 0F00Fh
        cmp     bl, 08h                 ; Even transparent?
        je      xparevn
        cmp     bh, 080h                ; Odd transparent?
        je      xparodd
        stosb
        loop    $L20008
        jmp     SHORT $FB397
xparevn:
        mov     al, BYTE PTR es:[di]
        and     al, 0Fh
        or      al, bh
        stosb
        loop    $L20008
        jmp     SHORT $FB397
xparodd:
        mov     al, BYTE PTR es:[di]
        and     al, 0F0h
        or      al, bl
        stosb
        loop    $L20008
        jmp     SHORT $FB397
nextpix:
        inc     di
        loop    $L20008
$FB397:
        add     di, WORD PTR [bp+16]    ; span
        dec     dx                      ; height
        jnz     $L20010
$L20021:
        pop     ds
        ASSUME  DS: DGROUP
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_spriteBuf      ENDP

_TEXT   ENDS
END
