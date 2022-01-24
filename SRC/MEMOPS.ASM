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
        imul    WORD PTR [bp+20]        ; span
        sar     di, 1
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
;    width >>= 1;
        sar     WORD PTR [bp+12], 1     ; width
;    do {
        mov     dx, WORD PTR [bp+14]    ; height
tilemem:
;        for (w = 0; w < width; w++)
;            buf[w] = tile[w];
        mov     cx, WORD PTR [bp+12]    ; width
        push    si
        push    di
        shr     cx, 1
    rep movsw
        jae     SHORT tmeven                  ; jump carry clear
        movsb
tmeven:
        pop     di
        pop     si
;        buf  += span;
        add     di, WORD PTR [bp+20]    ; span
;        tile += 8;
        add     si, 8                   ; tile
;    } while (--height);
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
        sar     di, 1
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
        sar     cx, 1
;    while (width--)
        mov     dx, cx
;        edgeH[0][x + width] = tile[width];
        sar     cx, 1
    rep movsw
        jae     SHORT th2even0
        movsb
th2even0:
;        edgeH[1][x + width] = tile[8 + width];
        sub     si, dx
        sub     di, dx
        add     si, 8
        add     di, 80
        mov     cx, dx
        sar     cx, 1
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
;    x     >>= 1;
        mov     di, WORD PTR [bp+4]     ; x
        sar     di, 1
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
        sar     cx, 1
;    while (width--)
;        edgeH[0][x + width] = tile[width];
        sar     cx, 1
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
;	y = 4
;	s = 6
;	t = 8
;	height = 10
;	tile = 12
;    tile += (t << 3) + (s >> 1);
        mov     di, WORD PTR [bp+8]     ; t
        shl     di, 1
        shl     di, 1
        shl     di, 1
        mov     cx, WORD PTR [bp+6]     ; s
        shr     cx, 1
        add     di, cx
        add     di, WORD PTR [bp+12]    ; OFFSET tile
        mov     es, WORD PTR [bp+14]    ; SEG tile
        mov     si, WORD PTR [bp+4]     ; y
;    while (height--)
        mov     cx, WORD PTR [bp+10]    ; height
tilememv:
;        edgeV[y++] = *tile;
;        tile      += 8;
        mov     al, BYTE PTR es:[di]
        add     di, 8
        mov     BYTE PTR _edgeV[si], al
        inc     si
        loop    tilememv
        pop     si
        pop     di
        pop     bp
        ret

_tileMemV	ENDP

PUBLIC	_spriteBuf

;void spriteBuf(int x, int y, int width, int height, unsigned char *spriteptr, int span, unsigned char *bufptr)
;{
;    int w;
;    unsigned char pix;
;
;    bufptr += y * span + (x >> 1);
;    width >>= 1;
;    span   -= width;
;    if (x & 1)
;    {
;        while (height--)
;        {
;            for (w = 0; w < width; w++)
;            {
;                pix = *spriteptr & 0x0F;
;                if (pix != 0x08)
;                    *bufptr = (*bufptr & 0x0F) | (pix << 4);
;                bufptr++;
;                pix = *spriteptr++ & 0xF0;
;                if (pix != 0x80)
;                    *bufptr = (*bufptr & 0xF0) | (pix >> 4);
;            }
;            bufptr += span;
;        }
;    }
;    else
;    {
;        while (height--)
;        {
;            for (w = 0; w < width; w++)
;            {
;                pix = *spriteptr & 0x0F;
;                if (pix != 0x08)
;                    *bufptr = (*bufptr & 0xF0) | pix;
;                pix = *spriteptr++ & 0xF0;
;                if (pix != 0x80)
;                    *bufptr = (*bufptr & 0x0F) | pix;
;                bufptr++;
;            }
;            bufptr += span;
;        }
;    }
;}

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
        mov     ax, WORD PTR [bp+6]     ; y
        imul    WORD PTR [bp+14]        ; span
        mov     cx, WORD PTR [bp+4]     ; x
        sar     cx, 1
        add     ax, cx
        add     WORD PTR [bp+16], ax    ; bufptr
        sar     WORD PTR [bp+8], 1      ; width
        mov     ax, WORD PTR [bp+8]     ; width
        sub     WORD PTR [bp+14],ax     ; span
        test    BYTE PTR [bp+4], 1      ; x
        je      $I382
        mov     si, WORD PTR [bp+16]    ; bufptr
        jmp     SHORT $L20007
$L20005:
;        mov     WORD PTR [bp-4], 0      ; w
        cmp     WORD PTR [bp+8], 0      ; width
        jle     $FB388
        mov     ax, WORD PTR [bp+8]     ; width
        mov     WORD PTR [bp-6], ax
;        add     WORD PTR [bp-4], ax     ; w
        mov     di, WORD PTR [bp+12]    ; spriteptr
$L20003:
        mov     dl, BYTE PTR [di]
        and     dl, 0Fh
        cmp     dl, 8                   ; Transparent?
        je      $I389
        mov     al, BYTE PTR [si]
        and     al, 0Fh
        mov     bx, dx
        mov     cl, 4
        shl     bl, cl
        or      al, bl
        mov     BYTE PTR [si], al
$I389:
        inc     si
        mov     dl, BYTE PTR [di]
        and     dl,0F0h
        inc     di
        cmp     dl, 128
        je      $I390
        mov     ax, dx
        sub     ah, ah
        mov     cl,4
        shr     ax, cl
        mov     cl, BYTE PTR [si]
        and     cl, 0F0h
        or      al, cl
        mov     BYTE PTR [si], al
$I390:
        dec     WORD PTR [bp-6]
        jne     $L20003
        mov     WORD PTR [bp+12], di    ; spriteptr
;        mov     BYTE PTR [bp-2], dl     ; pix
$FB388:
        add     si, WORD PTR [bp+14]    ; span
$L20007:
        mov     ax, WORD PTR [bp+10]    ; height
        dec     WORD PTR [bp+10]        ; height
        or      ax, ax
        jne     $L20005
        jmp     SHORT $L20021

$I382:
        mov     si, WORD PTR [bp+16]    ; bufptr
        jmp     SHORT $L20012
$L20010:
;        mov     WORD PTR [bp-4], 0      ; w
        cmp     WORD PTR [bp+8], 0      ; width
        jle     $FB397
        mov     dx, WORD PTR [bp+8]     ; width
;        add     WORD PTR [bp-4], dx     ; w
        mov     di, WORD PTR [bp+12]    ; spriteptr
$L20008:
        mov     cl ,BYTE PTR [di]
        and     cl, 0Fh
        cmp     cl, 8                   ; Transparent?
        je      $I398
        mov     al, BYTE PTR [si]
        and     al, 0F0h
        or      al, cl
        mov     BYTE PTR [si], al
$I398:
        mov     cl, BYTE PTR [di]
        and     cl, 0F0h
        inc     di
        cmp     cl, 128
        je      $I399
        mov     al, BYTE PTR [si]
        and     al, 0Fh
        or      al, cl
        mov     BYTE PTR [si], al
$I399:
        inc     si
        dec     dx
        jne     $L20008
        mov     WORD PTR [bp+12], di    ; spriteptr
;        mov     BYTE PTR [bp-2], cl     ; pix
$FB397:
        add     si, WORD PTR [bp+14]    ; span
$L20012:
        mov     ax, WORD PTR [bp+10]    ; height
        dec     WORD PTR [bp+10]        ; height
        or      ax, ax
        jne     $L20010
$L20021:
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret

_spriteBuf      ENDP

_TEXT   ENDS
END
