_TEXT   SEGMENT  WORD PUBLIC 'CODE'
_TEXT   ENDS
_DATA   SEGMENT  WORD PUBLIC 'DATA'
_DATA   ENDS
CONST   SEGMENT  WORD PUBLIC 'CONST'
CONST   ENDS
_BSS    SEGMENT  WORD PUBLIC 'BSS'
_BSS    ENDS
DGROUP  GROUP	CONST, _BSS, _DATA
        ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP

EXTRN   _scanline:BYTE
EXTRN   _edgeH:BYTE
EXTRN   _edgeV:BYTE
EXTRN   _tileMap:BYTE
EXTRN   _spanMap:BYTE

_TEXT   SEGMENT
        ASSUME	CS: _TEXT
        PUBLIC	_tileMem

;void tileMem(int x, int y, unsigned int s, unsigned int t, int width, int height, unsigned char far *tile, int span, unsigned char far *buf)
_tileMem        PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
; x             = 4
; y             = 6
; s             = 8
; t             = 10
; width         = 12
; height        = 14
; tile          = 16
; span          = 20
; buf           = 22
;    buf    += y * span + (x >> 1);
        mov     ax, WORD PTR [bp+6]     ; y
        mov     di, WORD PTR [bp+4]	    ; x
        shr     di, 1
        imul    WORD PTR [bp+20]        ; span
        add     di, ax
        add     di, WORD PTR [bp+22]    ; OFFSET buf
        mov     es, WORD PTR [bp+24]    ; SEG buf
;    tile   += t * 8 + (s >> 1);
        mov     si, WORD PTR [bp+10]    ; t
        shl     si, 1
        shl     si, 1
        shl     si, 1
        mov     cx, WORD PTR [bp+8]     ;s
        shr     cx, 1
        add     si, cx
        add     si, WORD PTR [bp+16]    ; OFFSET tile
        mov     ds, WORD PTR [bp+18]    ; SEG TILE
        ASSUME DS: NOTHING
        mov     bx, WORD PTR [bp+12]    ; width
        shr     bx, 1
        sub     WORD PTR [bp+20], bx    ; span
        mov     dx, WORD PTR [bp+14]    ; height
tilemem:
        mov     cx, bx
        shr     cx, 1
    rep movsw
        jae     SHORT tmeven            ; jump carry clear
        movsb
tmeven:
        add     di, WORD PTR [bp+20]    ; span
        add     si, 8                   ; tile
        sub     si, bx
        dec     dx                      ; height
        jne     tilemem
        pop     ds
        ASSUME DS: DGROUP
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_tileMem        ENDP

        PUBLIC  _tileMemH2
;void tileMemH2(int x, unsigned int s, unsigned int t, int width, unsigned char far *tile)
_tileMemH2      PROC NEAR
        push    bp
        mov     bp,sp
        sub     sp,2
        push    di
        push    si
        push    ds
;	x = 4
;	s = 6
;	t = 8
;	width = 10
;	tile = 12
;    x     >>= 1;
        mov     di, WORD PTR [bp+4]     ; x
        shr     di, 1
        add     di, OFFSET DGROUP:_edgeH
        mov     ax, ds
        mov     es, ax
;    tile   += (t << 3) + (s >> 1);
        mov     si, WORD PTR [bp+8]     ; t
        shl     si, 1
        shl     si, 1
        shl     si, 1
        mov     cx, WORD PTR [bp+6]     ; s
        shr     cx, 1
        add     si, cx
        add     si, WORD PTR [bp+12]    ; OFFSET tile
        mov     ds, WORD PTR [bp+14]    ; SEG tile
        ASSUME DS: NOTHING
;    width >>= 1;
        mov     bx, WORD PTR [bp+10]    ; width
        shr     bx, 1
;    while (width--)
        mov     cx, bx
;        edgeH[0][x + width] = tile[width];
        shr     cx, 1
    rep movsw
        jae     SHORT th2even0
        movsb
th2even0:
;        edgeH[1][x + width] = tile[8 + width];
        sub     si, bx
        sub     di, bx
        add     si, 8
        add     di, 80
        mov     cx, bx
        shr     cx, 1
    rep movsw
        jae     SHORT th2even1
        movsb
th2even1:
        pop     ds
        ASSUME  DS: DGROUP
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_tileMemH2      ENDP
PUBLIC	_tileMemH
_tileMemH	PROC NEAR
        push    bp
        mov     bp,sp
        push    di
        push    si
        push    ds
;	x = 4
;	s = 6
;	t = 8
;	width = 10
;	tile = 12
        mov     di, WORD PTR [bp+4]     ; x
        shr     di, 1
        add     di, OFFSET DGROUP:_edgeH
        mov     ax, ds
        mov     es, ax
;    tile   += (t << 3) + (s >> 1);
        mov     si, WORD PTR [bp+8]     ; t
        shl     si, 1
        shl     si, 1
        shl     si, 1
        mov     cx, WORD PTR [bp+6]     ; s
        shr     cx, 1
        add     si, cx
        add     si, WORD PTR [bp+12]    ; OFFSET tile
        mov     ds, WORD PTR [bp+14]    ; SEG tile
        ASSUME DS: NOTHING
;    width >>= 1;
        mov     cx, WORD PTR [bp+10]    ; width
        shr     cx, 1
;    while (width--)
;        edgeH[0][x + width] = tile[width];
        shr     cx, 1
    rep movsw
        jae     theven
        movsb
theven:
        pop     ds
        ASSUME  DS: DGROUP
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_tileMemH	ENDP

        PUBLIC  _tileMemV
; void tileMemV(int y, unsigned int s, unsigned int t, int height, unsigned char far *tile)
_tileMemV       PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
;	y = 4
;	s = 6
;	t = 8
;	height = 10
;	tile = 12
        mov     di, WORD PTR [bp+4]     ; y
        add     di, OFFSET DGROUP:_edgeV
        mov     ax, ds
        mov     es, ax
;    tile += (t << 3) + (s >> 1);
        mov     si, WORD PTR [bp+8]     ; t
        shl     si, 1
        shl     si, 1
        shl     si, 1
        mov     cx, WORD PTR [bp+6]     ; s
        shr     cx, 1
        add     si, cx
        add     si, WORD PTR [bp+12]    ; OFFSET tile
        mov     ds, WORD PTR [bp+14]    ; SEG tile
        ASSUME  DS: NOTHING
;    while (height--)
        mov     cx, WORD PTR [bp+10]    ; height
        shr     cx, 1
        jae     tilememv
        movsb
        jz      tilememvx
        add     si, 7
tilememv:
        movsb
        add     si, 7
        movsb
        dec     cx
        jz      SHORT tilememvx
        add     si, 7
        movsb
        add     si, 7
        movsb
        add     si, 7
        loop    tilememv
tilememvx:
        pop     ds
        ASSUME  DS: DGROUP
        pop     si
        pop     di
        pop     bp
        ret

_tileMemV	ENDP

PUBLIC	_spriteBuf

;void spriteBuf(int x, int y, int width, int height, unsigned char *spriteptr, int span, unsigned char *bufptr)
_spriteBuf      PROC NEAR
        push    bp
        mov     bp, sp
;        sub     sp, 6
        push    di
        push    si
;	x = 4
;	y = 6
;	width = 8
;	height = 10
;	spriteptr = 12
;	span = 14
;	bufptr = 16
;	w = -4
;	pix = -2
;    bufptr += y * span + (x >> 1);
        mov     ax, WORD PTR [bp+6]     ; y
        mov     cx, WORD PTR [bp+4]     ; x
        shr     cx, 1
        imul    WORD PTR [bp+14]        ; span
        add     ax, cx
        add     WORD PTR [bp+16], ax    ; bufptr
        ;    width >>= 1;
        sar     WORD PTR [bp+8], 1      ; width
;    span   -= width;
        mov     ax, WORD PTR [bp+8]     ; width
        sub     WORD PTR [bp+14],ax     ; span
;    if (x & 1)
;    {
        test    BYTE PTR [bp+4], 1      ; x
        je      $I382
;        while (height--)
;        {
        mov     di, WORD PTR [bp+12]    ; spriteptr
        mov     si, WORD PTR [bp+16]    ; bufptr
        jmp     SHORT $L20007
$L20005:
;            for (w = 0; w < width; w++)
;            {
;        mov     WORD PTR [bp-4], 0      ; w
        cmp     WORD PTR [bp+8], 0      ; width
        jle     $FB388
        mov     ax, WORD PTR [bp+8]     ; width
        mov     WORD PTR [bp-6], ax
;        add     WORD PTR [bp-4], ax     ; w
;                pix = *spriteptr & 0x0F;
        mov     dl, BYTE PTR [di]
        and     dl, 0Fh
$L20003:
;                if (pix != 0x08)
        cmp     dl, 8                   ; Transparent?
        je      $I389
;                    *bufptr = (*bufptr & 0x0F) | (pix << 4);
        mov     al, BYTE PTR [si]
        and     al, 0Fh
        mov     bx, dx
        mov     cl, 4
        shl     bl, cl
        or      al, bl
        mov     BYTE PTR [si], al
$I389:
;                bufptr++;
        inc     si
;                pix = *spriteptr++ & 0xF0;
        mov     dl, BYTE PTR [di]
        and     dl,0F0h
        inc     di
;                if (pix != 0x80)
        cmp     dl, 128
        je      $I390
;                    *bufptr = (*bufptr & 0xF0) | (pix >> 4);
        mov     ax, dx
        sub     ah, ah
        mov     cl,4
        shr     ax, cl
        mov     cl, BYTE PTR [si]
        and     cl, 0F0h
        or      al, cl
        mov     BYTE PTR [si], al
$I390:
;            }
        dec     WORD PTR [bp-6]
        jne     $L20003
        mov     WORD PTR [bp+12], di    ; spriteptr
;        mov     BYTE PTR [bp-2], dl     ; pix
$FB388:
;            bufptr += span;
        add     si, WORD PTR [bp+14]    ; span
$L20007:
        mov     ax, WORD PTR [bp+10]    ; height
        dec     WORD PTR [bp+10]        ; height
        or      ax, ax
        jne     $L20005
        jmp     SHORT $L20021
;        }
;    }

$I382:
;    else
        mov     si, WORD PTR [bp+12]    ; spriteptr
        mov     di, WORD PTR [bp+16]    ; bufptr
        mov     ax, ds
        mov     es, ax
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
        add     di, WORD PTR [bp+14]    ; span
$L20012:
        dec     dx                      ; height
        jnz     $L20010
$L20021:
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret

_spriteBuf      ENDP

_TEXT   ENDS
END
