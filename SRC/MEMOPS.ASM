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
        jae     tmeven                  ; jump carry clear
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
        jae     th2even0
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
        jae     th2even1
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

_TEXT   ENDS
END
