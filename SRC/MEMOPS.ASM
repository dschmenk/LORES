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
EXTRN   _spanMap:WORD

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

        PUBLIC  _tileEdgeH
_tileEdgeH      PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
;	s = 4
;	t = 6
;	tileptr = 8
;	x = -2
;   es:di => &edgeH[x]
;   dx:bx => tileptr
;   ds:si => *tileptr
        push    ds
        mov     ax, ds
        mov     es, ax
        ASSUME  DS: NOTHING, ES: DGROUP
        mov     di, OFFSET DGROUP:_edgeH
        mov     bx, WORD PTR [bp+8]         ; OFFSET tileptr
        mov     dx, WORD PTR [bp+10]        ; SEG tileptr
; tileMemH(0, s, t, 16 - s, *tileptr++);
        mov     ds, dx
        lds     si, [bx]                    ; *tileptr
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
@@:     add     bx, 4                       ; tileptr++
; for (x = 16 - s; x < 160 - 16; x += 16)
tileedgeh:
; tileMemH(x, 0, t, 16, *tileptr++);
        mov     ds, dx
        lds     si, [bx]                    ; *tileptr
        add     si, WORD PTR [bp+6]         ; t << 3
        mov     cx, 8/2
    rep movsw
        add     bx, 4                       ; tileptr++;
        cmp     di, OFFSET DGROUP:_edgeH + (160 - 16) / 2
        jb      tileedgeh
; tileMemH(x, 0, t, 160 - x, *tileptr++);
        mov     ds, dx
        lds     si, [bx]                    ; *tileptr
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
;	s = 4
;	t = 6
;	tileptr = 8
;	x = -2
;   es:di => &edgeH[x]
;   dx:bx => tileptr
;   ds:si => *tileptr
        push    ds
        mov     ax, ds
        mov     es, ax
        ASSUME  DS: NOTHING, ES: DGROUP
        mov     di, OFFSET DGROUP:_edgeH
;
; Even row
;
        mov     bx, WORD PTR [bp+8]         ; OFFSET tileptr
        mov     dx, WORD PTR [bp+10]        ; SEG tileptr
; tileMemH2(0, s, t, 16 - s, *tileptr++);
        mov     ds, dx
        lds     si, [bx]                    ; *tileptr
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
        add     bx, 4                       ; tileptr++
; for (x = 16 - s; x < 160 - 16; x += 16)
tileedgeh2:
; tileMemH2(x, 0, t, 16, *tileptr++);
        mov     ds, dx
        lds     si, [bx]                    ; *tileptr
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
        add     bx, 4                       ; tileptr++;
        cmp     di, OFFSET DGROUP:_edgeH + (160 - 16) / 2
        jb      tileedgeh2
; tileMemH2(x, 0, t, 160 - x, *tileptr++);
        mov     ds, dx
        lds     si, [bx]                    ; *tileptr
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

; void tileEdgeV(unsigned int s, unsigned int t, unsigned char far * far *tileptr)
_tileEdgeV      PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
;	s = bp+4
;	t = bp+6
;	tileptr = bp+8
;   es:di => &edgeV[y]
;   dx:bx => tileptr
;   ds:si => *tileptr
        push    ds
        mov     ax, ds
        mov     es, ax
        ASSUME  DS: NOTHING, ES: DGROUP
        mov     di, OFFSET DGROUP:_edgeV
        mov     bx, WORD PTR [bp+8]         ; OFFSET tileptr
        mov     dx, WORD PTR [bp+10]        ; SEG tileptr
; tileMemV(0, s, t, 16 - t, *tileptr);
        mov     ds, dx
        lds     si, [bx]                    ; *tileptr
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
; tileptr += spanMap;
        mov     ax, DGROUP: es:_spanMap
        shl     ax, 1
        shl     ax, 1
        add     bx, ax                      ; tileptr
; for (y = 16 - t; y < 100 - 16; y += 16)
tileedgev:
; tileMemV(y, s, 0, 16, *tileptr);
        mov     ds, dx
        lds     si, [bx]                    ; *tileptr
        add     si, WORD PTR [bp+4]         ; s
        REPT    16
        movsb
        add     si, 7
        ENDM
; tileptr += spanMap;
        mov     ax, DGROUP: es:_spanMap
        shl     ax, 1
        shl     ax, 1
        add     bx, ax                      ; tileptr
        cmp     di, OFFSET DGROUP:_edgeV + 100 - 16
        jb      tileedgev
; tileMemV(y, s, 0, 100 - y, *tileptr);
        mov     ds, dx
        lds     si, [bx]                    ; *tileptr
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

_tileEdgeV	ENDP

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
