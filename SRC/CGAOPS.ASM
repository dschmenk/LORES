_TEXT	SEGMENT  WORD PUBLIC 'CODE'
_TEXT	ENDS
_DATA	SEGMENT  WORD PUBLIC 'DATA'
_DATA	ENDS
CONST	SEGMENT  WORD PUBLIC 'CONST'
CONST	ENDS
_BSS	SEGMENT  WORD PUBLIC 'BSS'
_BSS	ENDS
DGROUP	GROUP	CONST, _BSS, _DATA
        ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP

EXTRN   _scanline:      WORD
EXTRN   _scrnMask:      WORD
EXTRN   _tileMap:       DWORD
EXTRN   _widthMap:      WORD
EXTRN   _spanMap:       WORD
EXTRN   _orgS:          WORD
EXTRN   _orgT:          WORD
EXTRN   _extS:          WORD
EXTRN   _extT:          WORD
EXTRN   _orgAddr:       WORD
EXTRN   _edgeH:         BYTE
EXTRN   _edgeV:         BYTE
EXTRN   _rasterTimer:   BYTE

_DATA   SEGMENT

cpybufsel       DW  cpybufh16, cpybufh1,  cpybufh2,  cpybufh3
                DW  cpybufh4,  cpybufh5,  cpybufh6,  cpybufh7
                DW  cpybufh8,  cpybufh9,  cpybufh10, cpybufh11
                DW  cpybufh12, cpybufh13, cpybufh14, cpybufh15

_DATA   ENDS

_TEXT   SEGMENT

        PUBLIC  _setStartAddr
_setStartAddr   PROC NEAR
        push    bp
        mov     bp, sp
        mov     bx, WORD PTR [bp+4]     ;startaddr
        mov     sp, bp
        pop     bp
        mov     dx, 03DAh               ; CGA status
        mov     BYTE PTR DGROUP:_rasterTimer, 1
waitact:                                ; Wait for end of active video
        mov     al, DGROUP:_rasterTimer
        or      al, al
        jnz     waitact
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        mov     dx, 03D4h               ; Update CGA start address
        mov     al, 12
        mov     ah, bh
        out     dx, ax
        inc     al
        mov     ah, bl
        out     dx, ax
        ret
_setStartAddr   ENDP

        PUBLIC  _cpyEdgeH
_cpyEdgeH       PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        mov     ax, 0B800H
        mov     es, ax
        mov     di, WORD PTR [bp+4]     ; pixaddr
        mov     si, OFFSET DGROUP:_edgeH
        mov     cx, WORD PTR [bp+6]     ; hcount
cpyedgeh:
        REPT    80
        movsb
        inc     di
        ENDM
        loop    @F
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
@@:     jmp     cpyedgeh
_cpyEdgeH       ENDP

        PUBLIC  _cpyEdgeV
_cpyEdgeV       PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        mov     ax, 0B800H
        mov     es, ax
        mov     di, WORD PTR [bp+4]     ; pixaddr
        mov     si, OFFSET DGROUP:_edgeV
cpyedgev:
        REPT    99
        movsb
        add     di, 159
        ENDM
        movsb
@@:     pop     si                      ; Note that this can be called from _cpyBuf
        pop     di
        mov     sp, bp
        pop     bp
        ret
_cpyEdgeV       ENDP
;
; Copy buffer to screen
;
; void _cpyBuf(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf)
        PUBLIC  __cpyBuf
__cpyBuf         PROC NEAR
        push    bp
        mov     bp, sp
        sub     sp, 8
        push    di
        push    si
; s             = bp+4
; t             = bp+6
; width         = bp+8
; height        = bp+10
; buf           = bp+12
; span          = bp-2
;
; Quick reject
;
; if ((s >= extS) || (s + width  <= orgS) || (t >= extT) || (t + height <= orgT))
        mov     ax, WORD PTR [bp+4]     ; s
        cmp     ax, _extS               ; s >= extS
        jae     @B
        mov     cx, WORD PTR [bp+8]     ; width
        add     ax, cx
        cmp     ax, DGROUP:_orgS
        jbe     @B
        mov     dx, WORD PTR [bp+6]     ; t
        cmp     dx, DGROUP:_extT        ; t >= extT
        jae      @B
        mov     ax, WORD PTR [bp+10]    ; height
        add     ax, WORD PTR [bp+6]     ; t
        cmp     ax, DGROUP:_orgT
        jbe     @B
; span = width >> 1;
        shr     cx, 1
        mov     WORD PTR [bp-2], cx     ; span
;
; Clip to screen edges
;
; if (s < orgS)
        mov     ax, DGROUP:_orgS
        cmp     WORD PTR [bp+4], ax     ; s
        jae     @F
; width -= orgS - s;
        mov     si, ax
        sub     si, WORD PTR [bp+4]     ; s
        sub     WORD PTR [bp+8], si     ; width
; buf += (orgS - s) >> 1;
        shr     si, 1
        add     WORD PTR [bp+12], si    ; buf
; s = orgS;
        mov     WORD PTR [bp+4], ax     ; s
; if (s + width > extS)
@@:     mov     ax, WORD PTR [bp+8]     ; width
        add     ax, WORD PTR [bp+4]     ; s
        cmp     ax, DGROUP:_extS        ; extS
        jbe     cbclipt
; width = extS - s;
        mov     ax, DGROUP:_extS        ; extS
        sub     ax, WORD PTR [bp+4]     ; s
        mov     WORD PTR [bp+8], ax     ; width
; if (t < orgT)
cbclipt:
        mov     ax, DGROUP:_orgT
        cmp     WORD PTR [bp+6], ax     ; t
        jae     @F
; height -= orgT - t;
        mov     si, ax
        sub     ax, WORD PTR [bp+6]     ; t
        sub     WORD PTR [bp+10], ax    ; height
; buf += (orgT - t) * span;
        mul     WORD PTR [bp-2]         ; span
        add     WORD PTR [bp+12], ax    ; buf
; t = orgT;
        mov     WORD PTR [bp+6], si     ; t
; if (t + height > extT)
@@:     mov     ax, WORD PTR [bp+10]    ; height
        add     ax, WORD PTR [bp+6]     ; t
        cmp     ax, DGROUP:_extT        ; extT
        jbe     cpybuf
; height = extT - t;
        mov     ax, _extT               ; extT
        sub     ax, WORD PTR [bp+6]     ; t
        mov     WORD PTR [bp+10], ax    ; height
; CPYBUF((scanline[t - orgT] + (s - orgS) + orgAddr) & 0x3FFF, width >> 1, height, span, buf);
cpybuf:
        push    ds
        mov     ax, 0B800H
        mov     es, ax
        mov     bx, WORD PTR [bp+6]     ; t
        sub     bx, DGROUP:_orgT
        shl     bx, 1
        mov     di, DGROUP:_scanline[bx]
        add     di, WORD PTR [bp+4]     ; s
        sub     di, DGROUP:_orgS
        add     di, DGROUP:_orgAddr     ; pixaddr = (scanline[t - orgT] + (s - orgS) + orgAddr) & 0x3FFF
        and     di, DGROUP:_scrnMask
        mov     cx, WORD PTR [bp+8]     ; width
        mov     bx, cx
        shr     cx, 1                   ; width/2
        mov     dl, cl                  ; width/2 must be less than 80
        mov     dh, BYTE PTR [bp+10]    ; height must be 100 or less
        and     bx, 01Eh
        mov     bx, cpybufsel[bx]
        lds     si, DWORD PTR [bp+12]   ; buf
        ASSUME  DS: NOTHING
        push    bp                      ; This is bad. Very bad
        mov     bp, WORD PTR [bp-2]     ; span
        sub     bp, cx                  ; span -= width/2
cpybufv:
        add     cx, 15                  ; Round up iteration count
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        jmp     bx
cpybufh16:
        movsb
        inc     di
cpybufh15:
        movsb
        inc     di
cpybufh14:
        movsb
        inc     di
cpybufh13:
        movsb
        inc     di
cpybufh12:
        movsb
        inc     di
cpybufh11:
        movsb
        inc     di
cpybufh10:
        movsb
        inc     di
cpybufh9:
        movsb
        inc     di
cpybufh8:
        movsb
        inc     di
cpybufh7:
        movsb
        inc     di
cpybufh6:
        movsb
        inc     di
cpybufh5:
        movsb
        inc     di
cpybufh4:
        movsb
        inc     di
cpybufh3:
        movsb
        inc     di
cpybufh2:
        movsb
        inc     di
cpybufh1:
        movsb
        inc     di
        loop    cpybufh16
        add     di, 160
        mov     cl, dl                  ; width/2
        sub     di, cx
        sub     di, cx
        add     si, bp                  ; span
        dec     dh                      ; height
        jnz     cpybufv
        pop     bp
        pop     ds
        ASSUME  DS: DGROUP
@@:     pop     si                      ; Note that this can be called from _cpyBufSnow
        pop     di
        mov     sp, bp
        pop     bp
        ret
__cpyBuf         ENDP
;
; Copy buffer to screen with snow checking
;
; void _cpyBufSnow(unsigned int s, unsigned int t, int width, int height, unsigned char far *buf)
        PUBLIC  __cpyBufSnow
__cpyBufSnow     PROC NEAR
        push    bp
        mov     bp, sp
        sub     sp, 8
        push    di
        push    si
; s             = bp+4
; t             = bp+6
; width         = bp+8
; height        = bp+10
; buf           = bp+12
; span          = bp-2
; if ((s >= extS) || (s + width  <= orgS) || (t >= extT) || (t + height <= orgT))
        mov     ax, WORD PTR [bp+4]     ; s
        cmp     ax, DGROUP:_extS        ; s >= extS
        jae     @B
        mov     cx, WORD PTR [bp+8]     ; width
        add     ax, cx
        cmp     ax, DGROUP:_orgS
        jbe     @B
        mov     dx, WORD PTR [bp+6]     ; t
        cmp     dx, DGROUP:_extT        ; t >= extT
        jae     @B
        mov     ax, WORD PTR [bp+10]    ; height
        add     ax, WORD PTR [bp+6]     ; t
        cmp     ax, DGROUP:_orgT
        jbe     @B
; span = width >> 1;
        shr     cx, 1
        mov     WORD PTR [bp-2], cx     ; span
;
; Clip to screen edges
;
; if (s < orgS)
        mov     ax, DGROUP:_orgS
        cmp     WORD PTR [bp+4], ax     ; s
        jae     @F
; width -= orgS - s;
        mov     si, ax
        sub     si, WORD PTR [bp+4]     ; s
        sub     WORD PTR [bp+8], si     ; width
; buf += (orgS - s) >> 1;
        shr     si, 1
        add     WORD PTR [bp+12], si    ; buf
; s = orgS;
        mov     WORD PTR [bp+4], ax     ; s
; if (s + width > extS)
@@:     mov     ax, WORD PTR [bp+8]     ; width
        add     ax, WORD PTR [bp+4]     ; s
        cmp     ax, DGROUP:_extS        ; extS
        jbe     cbsclipt
; width = extS - s;
        mov     ax, DGROUP:_extS        ; extS
        sub     ax, WORD PTR [bp+4]     ; s
        mov     WORD PTR [bp+8], ax     ; width
; if (t < orgT)
cbsclipt:
        mov     ax, DGROUP:_orgT
        cmp     WORD PTR [bp+6], ax     ; t
        jae     @F
; height -= orgT - t;
        mov     si, ax
        sub     ax, WORD PTR [bp+6]     ; t
        sub     WORD PTR [bp+10], ax    ; height
; buf += (orgT - t) * span;
        mul     WORD PTR [bp-2]         ; span
        add     WORD PTR [bp+12], ax    ; buf
; t = orgT;
        mov     WORD PTR [bp+6], si     ; t
; if (t + height > extT)
@@:     mov     ax, WORD PTR [bp+10]    ; height
        add     ax, WORD PTR [bp+6]     ; t
        cmp     ax, DGROUP:_extT        ; extT
        jbe     cpybufsnow
; height = extT - t;
        mov     ax, DGROUP:_extT        ; extT
        sub     ax, WORD PTR [bp+6]     ; t
        mov     WORD PTR [bp+10], ax    ; height
; CPYBUF((scanline[t - orgT] + (s - orgS) + orgAddr) & 0x3FFF, width >> 1, height, span, buf);
cpybufsnow:
        push    ds
        mov     ax, 0B800H
        mov     es, ax
        mov     bx, WORD PTR [bp+6]     ; t
        sub     bx, DGROUP:_orgT
        shl     bx, 1
        mov     di, DGROUP:_scanline[bx]
        add     di, WORD PTR [bp+4]     ; s
        sub     di, DGROUP:_orgS
        add     di, DGROUP:_orgAddr     ; pixaddr = (scanline[t - orgT] + (s - orgS) + orgAddr) & 0x3FFF
        and     di, DGROUP:_scrnMask
        lds     si, DWORD PTR [bp+12]   ; buf
        ASSUME  DS: NOTHING
        mov     bx, WORD PTR [bp+8]     ; width
        shr     bx, 1                   ; width/2
        sub     WORD PTR [bp-2], bx     ; span -= width/2
        mov     dx, 03DAh
cpybufsv:
        mov     cx, bx
        shr     cx, 1
        jnc     cpybufsh
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        movsb
        inc     di
        jcxz    cpybufsnxt
cpybufsh:
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        movsb
        inc     di
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        movsb
        inc     di
        loop    cpybufsh
cpybufsnxt:
        add     di, 160
        add     si, WORD PTR [bp-2]     ; span
        sub     di, bx
        sub     di, bx
        dec     WORD PTR [bp+10]        ; height
        jnz     cpybufsv
        pop     ds
        ASSUME  DS: DGROUP
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
__cpyBufSnow     ENDP

        ASSUME  DS:NOTHING

; void tileRow(int y, unsigned int s, unsigned int t, int height, unsigned char far * far *tileptr)
tileRow         PROC NEAR
; s             = bp+4
; t             = bp+6
; x             = bp-2
; y             = bp-4
; ss            = bp-6
; tilerow       = bp-10, bp-8
; tile          = bp-14, bp-12
; pixaddr       = bp-16
; width/2       = bp-18
; height        = bp-20
; x = 16 - s;
        mov     ax, WORD PTR [bp+4]     ; s
        shr     ax, 1
        mov     WORD PTR [bp-6], ax     ; ss = s/2
        shl     ax, 1
; x = 16 - s; // x is the width of the first tile and start of the second tile column
        sub     ax, 16
        neg     ax
        mov     WORD PTR [bp-2], ax     ; x
; tile(0, y, s, t, x, height, *tilerow++);
        shr     ax, 1
        mov     WORD PTR [bp-18], ax    ; width/2 = x/2
        call    tile
        add     WORD PTR [bp-10], 4     ; tilerow++
        mov     BYTE PTR [bp-6], 0      ; ss = 0
        mov     BYTE PTR [bp-18], 8     ; width/2 = 8
; pixaddr += x;
        mov     ax, WORD PTR [bp-2]     ; x
        add     WORD PTR [bp-16], ax    ; pixaddr
tilemidrow:
; tile(x, y, 0, t, 16, height, *tilerow++);
        mov     di, WORD PTR [bp-16]    ; pixaddr
        lds     si, DWORD PTR [bp-10]   ; tilerow
        lds     si, [si]                ; tile
        add     si, WORD PTR [bp+6]     ; t * 8
        mov     dx, WORD PTR [bp-20]    ; height
tilemidtile:
        REPT    8
        movsb
        inc     di
        ENDM
        add     di, 160-16
        dec     dx                      ; height
        jnz     tilemidtile
; tilerow++;
        add     WORD PTR [bp-10], 4     ; OFFSET tilerow
; x += 16;
        add     WORD PTR [bp-16], 16    ; pixaddr
        mov     ax, WORD PTR [bp-2]     ; x
        add     ax, 16
        mov     WORD PTR [bp-2], ax     ; x
; while (x < 160 - 16);
@@:     cmp     ax, 144
        jl      tilemidrow
; tile(x, y, 0, t, 160 - x, height, *tilerow);
        sub     ax, 160                 ; x - 160
        neg     ax                      ; -(x - 160)
        shr     ax, 1
        mov     WORD PTR [bp-18], ax    ; width/2 = (160 - x)/2
        mov     WORD PTR [bp-12], ax    ; SEG tile
;       jmp     tile                    ; Fall through
tileRow         ENDP

; void tile(int pixaddr, int width, int height, unsigned char far *buf)
tile            PROC NEAR
; s             = bp+4
; t             = bp+6
; x             = bp-2
; y             = bp-4
; ss            = bp-6
; tilerow       = bp-10, bp-8
; tile          = bp-14, bp-12
; pixaddr       = bp-16
; width/2       = bp-18
; height        = bp-20
; es:di => cga
; ds:si => tile
; bx => width/2
; dx => height
        mov     di, WORD PTR [bp-16]    ; pixaddr
; tile   += t * 8 + (s >> 1);
        lds     si, DWORD PTR [bp-10]   ; tilerow
        lds     si, [si]                ; tile
        add     si, WORD PTR [bp+6]     ; t * 8
        add     si, WORD PTR [bp-6]     ; ss = s/2
        mov     bx, WORD PTR [bp-18]    ; width/2
        mov     dx, WORD PTR [bp-20]    ; height
tilev:
        mov     cx, bx
        shr     cx, 1
        jnc     tileh
        movsb
        inc     di
        jcxz    tilenxt
tileh:
        movsb
        inc     di
        movsb
        inc     di
        loop    tileh
tilenxt:
        add     si, 8                   ; span
        sub     si, bx
        add     di, 160
        sub     di, bx
        sub     di, bx
        dec     dx                      ; height
        jnz     tilev
        ret
tile            ENDP

        ASSUME  DS: DGROUP

        PUBLIC  _tileScrn
; void tileScrn(unsigned int s, unsigned int t)
_tileScrn       PROC NEAR
        push    bp
        mov     bp, sp
        sub     sp, 24
        push    di
        push    si
; s             = bp+4
; t             = bp+6
; x             = bp-2
; y             = bp-4
; ss            = bp-6
; tilerow       = bp-10, bp-8
; tile          = bp-14, bp-12
; pixaddr       = bp-16
; width/2       = bp-18
; height        = bp-20
;
        mov     ax, 0B800H
        mov     es, ax
; tilerow = tileMap + ((t >> 4) * widthMap + (s >> 4)) * 4;
        mov     ax, WORD PTR [bp+6]     ; t
        and     al, 0F0h
        shr     ax, 1
        shr     ax, 1
        mul     WORD PTR DGROUP:_widthMap
        mov     dx, WORD PTR [bp+4]     ; s
        and     dl, 0F0h
        shr     dx, 1
        shr     dx, 1
        add     ax, dx
        add     ax, WORD PTR DGROUP:_tileMap
        mov     dx, WORD PTR DGROUP:_tileMap+2
        mov     WORD PTR [bp-10], ax    ; OFFSET tilerow
        mov     WORD PTR [bp-8], dx     ; SEG tilerow
; s &= 0x0F;
        and     WORD PTR [bp+4], 0Fh    ; s
; t &= 0x0F;
        mov     bx, WORD PTR [bp+6]     ; t
        and     bx, 000Fh
; y  = 16 - t; // y is the height of the first tile and start of second tile row
        mov     ax, 16
        sub     ax, bx
        shl     bx, 1
        shl     bx, 1
        shl     bx, 1
        mov     WORD PTR [bp+6], bx     ; t
        mov     WORD PTR [bp-4], ax     ; y
; tileRow(0, s, t, y, tileptr);
        mov     WORD PTR [bp-20], ax    ; height = y
        mov     ax, DGROUP:_orgAddr
        mov     WORD PTR [bp-16], ax    ; pixaddr = 0 + orgAddr
        push    WORD PTR [bp-10]        ; OFFSET tilerow
        push    ds
        call    tileRow
        pop     ds
; tilerow += spanMap;
        pop     ax
        add     ax, DGROUP:_spanMap
        mov     WORD PTR [bp-10], ax    ; OFFSET tilerow
        mov     BYTE PTR [bp+6], 0      ; t = 0
        mov     WORD PTR [bp-20], 16    ; height = 16
        mov     bx, WORD PTR [bp-4]     ; y
tilemidrows:
        shl     bx, 1
        mov     ax, DGROUP:_scanline[bx]
        add     ax, DGROUP:_orgAddr
        and     ax, DGROUP:_scrnMask
        mov     WORD PTR [bp-16], ax    ; pixaddr = (scanline[y] + orgAddr) & scrnMask
; tileRow(y, s, 0, 16, tileptr);
        push    WORD PTR [bp-10]        ; OFFSET tilerow
        push    ds
        call    tileRow
        pop     ds
; tilerow += spanMap;
        pop     ax
        add     ax, DGROUP:_spanMap
        mov     WORD PTR [bp-10], ax    ; OFFSET tilerow
; y += 16;
        mov     bx, WORD PTR [bp-4]     ; y
        add     bx, 16
        mov     WORD PTR [bp-4], bx     ; y
; while (y < 100 - 16);
        cmp     bx, 84
        jl      tilemidrows
; tileRow(y, s, 0, 100 - y, tileptr);
        mov     ax, 100
        sub     ax, bx
        mov     WORD PTR [bp-20], ax    ; height = 100 - y
        shl     bx, 1
        mov     ax, DGROUP:_scanline[bx]
        add     ax, DGROUP:_orgAddr
        and     ax, DGROUP:_scrnMask
        mov     WORD PTR [bp-16], ax    ; pixaddr = (scanline[y] + orgAddr) & scrnMask
        push    ds
        call    tileRow
        pop     ds
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_tileScrn       ENDP

        PUBLIC  _spriteScrn
;void spriteScrn(int x, int y, int width, int height, int span, unsigned char far *spriteImg)
_spriteScrn     PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
        ASSUME  DS: NOTHING
; x             = bp+4
; y             = bp+6
; width         = bp+8
; height        = bp+10
; span          = bp+12
; spriteptr     = bp+14, bp+16
        mov     ax, 0B800H
        mov     es, ax
        mov     bx, WORD PTR [bp+6]     ; y
        shl     bx, 1
        mov     di, DGROUP:_scanline[bx]
        mov     cx, WORD PTR [bp+4]     ; x
        and     cl, 0FEh
        add     di, cx
        add     di, DGROUP:_orgAddr
        and     di, DGROUP:_scrnMask
        shr     WORD PTR [bp+8], 1      ; width
        mov     cx, WORD PTR [bp+8]     ; width
        sub     WORD PTR [bp+12], cx    ; span
        mov     dx, WORD PTR [bp+10]    ; height
        lds     si, DWORD PTR [bp+14]   ; sprite
        test    BYTE PTR [bp+4], 1      ; x
        jnz     sprtdstodd
sprtevenx:
        lodsb
        cmp     al, 088h                ; Both nybbles transparent?
        je      sprtevennxt
        mov     bl, al
        mov     bh, al
        and     bx, 0F00Fh
        cmp     bl, 08h                 ; Low nybble transparent?
        je      sprtexparlo
        cmp     bh, 080h                ; High nybble transparent?
        je      sprtexparhi
        stosb
        inc     di
        loop    sprtevenx
        jmp     SHORT @F
sprtexparlo:
        mov     al, es:[di]
        and     al, 0Fh
        or      al, bh
        stosb
        inc     di
        loop    sprtevenx
        jmp     SHORT @F
sprtexparhi:
        mov     al, es:[di]
        and     al, 0F0h
        or      al, bl
        stosb
        inc     di
        loop    sprtevenx
        jmp     SHORT @F
sprtevennxt:
        inc     di
        inc     di
        loop    sprtevenx
@@:     mov     cx, WORD PTR [bp+8]     ; width
        add     di, 160
        sub     di, cx
        sub     di, cx
        add     si, WORD PTR [bp+12]    ; span
        dec     dx                      ; height
        jnz     sprtevenx
        jmp    sprtexit
sprtdstodd:
;        dec     WORD PTR [bp+12]        ; span
sprtoddfirst:
        xor     ax, ax
        lodsb
        ror     ax, 1
        ror     ax, 1
        ror     ax, 1
        ror     ax, 1
        cmp     ah, 80h;                ; High nybble transparent?
        je      @F
        mov     bh, es:[di]
        and     bh, 0Fh
        or      ah, bh
        mov     es:[di], ah
@@:     inc     di
        inc     di
        mov     bh, al
        loop    sprtoddx
        jmp     SHORT sprtoddlast
sprtoddx:
        xor     ax, ax
        lodsb
        ror     ax, 1
        ror     ax, 1
        ror     ax, 1
        ror     ax, 1
        mov     bl, bh
        or      bl, ah
        cmp     bl, 088h                ; Both nybbles transparent?
        je      sprtoddnxt
        cmp     bh, 08h                 ; Low nybble transparent?
        je      sprtoxparlo
        cmp     ah, 080h                ; High nybble transparent?
        je      sprtoxparhi
        mov     bh, al
        mov     al, bl
        stosb
        inc     di
        loop    sprtoddx
        jmp     SHORT sprtoddlast
sprtoxparlo:
        mov     bh, al
        mov     al, es:[di]
        and     al, 0Fh
        or      al, ah
        stosb
        inc     di
        loop    sprtoddx
        jmp     SHORT sprtoddlast
sprtoxparhi:
        xchg    bh, al
        mov     bl, es:[di]
        and     bl, 0F0h
        or      al, bl
        stosb
        inc     di
        loop    sprtoddx
        jmp     SHORT sprtoddlast
sprtoddnxt:
        mov     bh, al
        inc     di
        inc     di
        loop    sprtoddx
sprtoddlast:
        cmp     bh, 08h;                ; Low nybble transparent?
        je      @F
        mov     ah, es:[di]
        and     ah, 0F0h
        or      ah, bh
        mov     es:[di], ah
@@:     inc     di
        inc     di
        mov     cx, WORD PTR [bp+8]     ; width
        add     di, 160-2
        sub     di, cx
        sub     di, cx
        add     si, WORD PTR [bp+12]    ; span
        dec     dx                      ; height
        jz      sprtexit
        jmp     sprtoddfirst
sprtexit:
        pop     ds
        ASSUME  DS: DGROUP
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_spriteScrn     ENDP

        PUBLIC  __plot
; void _plot(unsigned int x, unsigned int y, unsigned char color)
__plot          PROC NEAR
        push    bp
        mov     bp, sp
        push    di
; x             = bp+4
; y             = bp+6
; color         = bp+8
        mov     ax, 0B800H
        mov     es, ax
        mov     bx, WORD PTR [bp+6]     ; y
        shl     bx, 1
        mov     di, WORD PTR _scanline[bx]
        mov     dx, WORD PTR [bp+4]     ; x
        mov     cx, dx
        and     cl, 0FEh
        add     di, cx
        add     di, _orgAddr
        and     di, DGROUP:_scrnMask
        mov     ah, BYTE PTR [bp+8]     ; color
        mov     al, es:[di]
        shr     dx, 1
        jnc     @F
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        and     al, 0Fh
        jmp     SHORT plotit
@@:     and     ax, 0FF0h
plotit:
        or      al, ah
        mov     es:[di], al
        pop     di
        mov     sp, bp
        pop     bp
        ret
__plot          ENDP

        PUBLIC  __plotSnow
; void _plotSnow(unsigned int x, unsigned int y, unsigned char color)
__plotSnow    PROC NEAR
        push    bp
        mov     bp, sp
        push    di
; x             = bp+4
; y             = bp+6
; color         = bp+8
        ;
        mov     ax, 0B800H
        mov     es, ax
        mov     bx, WORD PTR [bp+6]     ; y
        shl     bx, 1
        mov     di, WORD PTR _scanline[bx]
        mov     dx, WORD PTR [bp+4]     ; x
        mov     cx, dx
        and     dl, 0FEh
        add     di, cx
        add     di, _orgAddr
        and     di, DGROUP:_scrnMask
        mov     ah, BYTE PTR [bp+8]     ; color
        mov     dx, 03DAh
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        mov     al, es:[di]
        shr     cx, 1
        jnc     @F
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        and     ax, 0F00Fh
        jmp     SHORT   plotsit
@@:     and     ax, 0FF0h
plotsit:
        or      ah, al
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        mov     es:[di], ah
        pop     di
        mov     sp, bp
        pop     bp
        ret
__plotSnow      ENDP

        PUBLIC  __hlin
; void _hlin(unsigned int x1, unsigned int x2, unsigned int y, unsigned char color)
__hlin          PROC NEAR
        push    bp
        mov     bp, sp
        push    di
; x1            = bp+4
; x2            = bp+6
; y             = bp+8
; color         = bp+10
        mov     ax, 0B800H
        mov     es, ax
        mov     bx, WORD PTR [bp+8]     ; y
        shl     bx, 1
        mov     di, WORD PTR _scanline[bx]
        mov     dx, WORD PTR [bp+4]     ; x1
        mov     cx, dx
        and     cl, 0FEh
        add     di, cx
        add     di, _orgAddr
        and     di, DGROUP:_scrnMask
        mov     ah, BYTE PTR [bp+10]     ; color
        mov     al, ah
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        or      ah, al
        mov     cx, WORD PTR [bp+6]     ; x2
        sub     cx, dx
        jz      hlin_singl
        inc     cx
        shr     dx, 1
        jnc     hlin
        mov     al, es:[di]
        and     al, 0Fh
        mov     dh, ah
        and     dh, 0F0h
        or      al, dh
        stosb
        inc     di
        dec     cx
hlin:   mov     al, ah
        shr     cx, 1
        jz      hlin_last
@@:     stosb
        inc     di
        loop    @B
hlin_last:
        jnc     hlin_ex
@@:     mov     al, es:[di]
        and     ax, 0FF0h
        or      al, ah
        stosb
hlin_ex:
        pop     di
        mov     sp, bp
        pop     bp
        ret
hlin_singl:
        shr     dx, 1
        jnc     @B
        mov     al, es:[di]
        and     ax, 0F00Fh
        or      al, ah
        stosb
        jmp     hlin_ex
__hlin          ENDP

        PUBLIC  __hlinSnow
; void _hlinSnow(unsigned int x1, unsigned int x2, unsigned int y, unsigned char color)
__hlinSnow      PROC NEAR
        push    bp
        mov     bp, sp
        push    di
; x1            = bp+4
; x2            = bp+6
; y             = bp+8
; color         = bp+10
        mov     ax, 0B800H
        mov     es, ax
        mov     bx, WORD PTR [bp+8]     ; y
        shl     bx, 1
        mov     di, WORD PTR _scanline[bx]
        mov     dx, WORD PTR [bp+4]     ; x1
        mov     cx, dx
        and     cl, 0FEh
        add     di, cx
        add     di, _orgAddr
        and     di, DGROUP:_scrnMask
        mov     ah, BYTE PTR [bp+10]     ; color
        mov     al, ah
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        or      ah, al
        mov     cx, WORD PTR [bp+6]     ; x2
        sub     cx, dx
        jz      hlins_singl
        inc     cx
        shr     dx, 1
        mov     dx, 03DAh
        jnc     hlins
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        mov     bl, es:[di]
        and     bl, 0Fh
        mov     bh, ah
        and     bh, 0F0h
        or      bl, bh
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        mov     es:[di], bl
        inc     di
        inc     di
        dec     cx
hlins:  mov     bx, cx
        shr     cx, 1
        jz      hlins_last
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        mov     es:[di], ah
        inc     di
        inc     di
        loop    @B
        shr     bx, 1
hlins_last:
        jnc     hlins_ex
hlins_se:
        in      al, dx
        shr     al, 1
        jnc     hlins_se
        mov     bl, es:[di]
        and     bl, 0F0h
        and     ah, 0Fh
        or      bl, ah
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        mov     es:[di], bl
hlins_ex:
        pop     di
        mov     sp, bp
        pop     bp
        ret
hlins_singl:
        shr     dx, 1
        mov     dx, 03DAh
        jnc     hlins_se
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        mov     bl, es:[di]
        and     bl, 0Fh
        and     ah, 0F0h
        or      bl, ah
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        mov     es:[di], bl
        jmp     hlins_ex
__hlinSnow      ENDP

        PUBLIC  __vlin
; void _vlin(unsigned int x, unsigned int y1, unsigned int y2, unsigned char color)
__vlin          PROC NEAR
        push    bp
        mov     bp, sp
        push    di
; x             = bp+4
; y1            = bp+6
; y2            = bp+8
; color         = bp+10
        mov     ax, 0B800H
        mov     es, ax
        mov     bx, WORD PTR [bp+6]     ; y1
        shl     bx, 1
        mov     di, WORD PTR _scanline[bx]
        mov     dx, WORD PTR [bp+4]     ; x
        mov     cx, dx
        and     cl, 0FEh
        add     di, cx
        add     di, _orgAddr
        and     di, DGROUP:_scrnMask
        shr     bx, 1
        mov     cx, WORD PTR [bp+8]     ; y2
        sub     cx, bx
        inc     cx
        mov     ah, BYTE PTR [bp+10]    ; color
        mov     bl, 0F0h
        shr     dx, 1
        jnc     vlin
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        mov     bl, 0Fh
vlin:   mov     al, es:[di]
        and     al, bl
        or      al, ah
        mov     es:[di], al
        add     di, 160
        loop    vlin
        pop     di
        mov     sp, bp
        pop     bp
        ret
__vlin          ENDP

        PUBLIC  __vlinSnow
; void _vlinSnow(unsigned int x, unsigned int y1, unsigned int y2, unsigned char color)
__vlinSnow      PROC NEAR
        push    bp
        mov     bp, sp
        push    di
; x             = bp+4
; y1            = bp+6
; y2            = bp+8
; color         = bp+10
        mov     ax, 0B800H
        mov     es, ax
        mov     bx, WORD PTR [bp+6]     ; y1
        shl     bx, 1
        mov     di, WORD PTR _scanline[bx]
        mov     dx, WORD PTR [bp+4]     ; x
        mov     cx, dx
        and     cl, 0FEh
        add     di, cx
        add     di, _orgAddr
        and     di, DGROUP:_scrnMask
        shr     bx, 1
        mov     cx, WORD PTR [bp+8]     ; y2
        sub     cx, bx
        inc     cx
        mov     ah, BYTE PTR [bp+10]    ; color
        mov     bl, 0F0h
        shr     dx, 1
        mov     dx, 03DAh
        jnc     vlins
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        mov     bl, 0Fh
vlins:  in      al, dx
        shr     al, 1
        jnc     vlins
        mov     bh, es:[di]
        and     bh, bl
        or      bh, ah
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        mov     es:[di], bh
        add     di, 160
        loop    vlins
        pop     di
        mov     sp, bp
        pop     bp
        ret
__vlinSnow      ENDP

_TEXT	ENDS
END
