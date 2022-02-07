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
        cmp     ax, _orgS
        jbe     @B
        mov     dx, WORD PTR [bp+6]     ; t
        cmp     dx, _extT               ; t >= extT
        jae      @B
        mov     ax, WORD PTR [bp+10]    ; height
        add     ax, WORD PTR [bp+6]     ; t
        cmp     ax, _orgT
        jbe     @B
; span = width >> 1;
        shr     cx, 1
        mov     WORD PTR [bp-2], cx     ; span
;
; Clip to screen edges
;
; if (s < orgS)
        mov     ax, _orgS
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
        cmp     ax, _extS               ; extS
        jbe     cbclipt
; width = extS - s;
        mov     ax, _extS               ; extS
        sub     ax, WORD PTR [bp+4]     ; s
        mov     WORD PTR [bp+8], ax     ; width
; if (t < orgT)
cbclipt:
        mov     ax, _orgT
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
        cmp     ax, _extT               ; extT
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
        sub     bx, _orgT
        shl     bx, 1
        mov     di, WORD PTR _scanline[bx]
        add     di, WORD PTR [bp+4]     ; s
        sub     di, _orgS
        add     di, _orgAddr
        and     di, 03FFFh              ; pixaddr = (scanline[t - orgT] + (s - orgS) + orgAddr) & 0x3FFF
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
        cmp     ax, _extS               ; s >= extS
        jae     @B
        mov     cx, WORD PTR [bp+8]     ; width
        add     ax, cx
        cmp     ax, _orgS
        jbe     @B
        mov     dx, WORD PTR [bp+6]     ; t
        cmp     dx, _extT               ; t >= extT
        jae     @B
        mov     ax, WORD PTR [bp+10]    ; height
        add     ax, WORD PTR [bp+6]     ; t
        cmp     ax, _orgT
        jbe     @B
; span = width >> 1;
        shr     cx, 1
        mov     WORD PTR [bp-2], cx     ; span
;
; Clip to screen edges
;
; if (s < orgS)
        mov     ax, _orgS
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
        cmp     ax, _extS               ; extS
        jbe     cbsclipt
; width = extS - s;
        mov     ax, _extS               ; extS
        sub     ax, WORD PTR [bp+4]     ; s
        mov     WORD PTR [bp+8], ax     ; width
; if (t < orgT)
cbsclipt:
        mov     ax, _orgT
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
        cmp     ax, _extT               ; extT
        jbe     cpybufsnow
; height = extT - t;
        mov     ax, _extT               ; extT
        sub     ax, WORD PTR [bp+6]     ; t
        mov     WORD PTR [bp+10], ax    ; height
; CPYBUF((scanline[t - orgT] + (s - orgS) + orgAddr) & 0x3FFF, width >> 1, height, span, buf);
cpybufsnow:
        push    ds
        mov     ax, 0B800H
        mov     es, ax
        mov     bx, WORD PTR [bp+6]     ; t
        sub     bx, _orgT
        shl     bx, 1
        mov     di, WORD PTR _scanline[bx]
        add     di, WORD PTR [bp+4]     ; s
        sub     di, _orgS
        add     di, _orgAddr
        and     di, 03FFFh              ; pixaddr = (scanline[t - orgT] + (s - orgS) + orgAddr) & 0x3FFF
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

        PUBLIC  __tile
; void _tile(int pixaddr, int width, int height, unsigned char far *buf)
__tile          PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
; pixaddr       = bp+4
; width/2       = bp+6
; height        = bp+8
; buf           = bp+10
        mov     ax, 0B800H
        mov     es, ax
        mov     di, WORD PTR [bp+4]     ; pixaddr
        lds     si, DWORD PTR [bp+10]   ; pixbuf
        ASSUME  DS: NOTHING
        mov     bx, WORD PTR [bp+6]     ; width/2
        mov     dx, WORD PTR [bp+8]     ; height
tilev:
        mov     cx, bx
        shr     cx, 1
tileh:
        movsb
        inc     di
        movsb
        inc     di
        loop    tileh
        jnc     tilenxt
        movsb
        inc     di
tilenxt:
        add     si, 8                   ; span
        sub     si, bx
        add     di, 160
        sub     di, bx
        sub     di, bx
        dec     dx                      ; height
        jnz     tilev
        pop     ds
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
__tile          ENDP

        PUBLIC  __tileSnow
; void _tileSnow(int pixaddr, int width, int height, unsigned char far *buf)
__tileSnow    PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
; pixaddr       = bp+4
; width/2       = bp+6
; height        = bp+8
; buf           = bp+10
        mov     ax, 0B800H
        mov     es, ax
        mov     di, WORD PTR [bp+4]     ; pixaddr
        lds     si, DWORD PTR [bp+10]   ; pixbuf
        ASSUME  DS: NOTHING
        mov     bx, WORD PTR [bp+6]     ; width/2
        mov     dx, 03DAh
tilesv:
        mov     cx, bx
        shr     cx, 1
        jnc     tilesh
@@:     in      al, dx
        shr     al, 1
        jnc     @B
        movsb
        inc     di
        jcxz    tilesnxt
tilesh:
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
        loop    tilesh
tilesnxt:
        add     si, 8                   ; span
        sub     si, bx
        add     di, 160
        sub     di, bx
        sub     di, bx
        dec     WORD PTR [bp+8]         ; height
        jnz     tilesv
        pop     ds
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
__tileSnow      ENDP

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
        add     di, dx
        or      di, 0001h
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
        add     di, dx
        or      di, 0001h
        mov     ah, BYTE PTR [bp+8]     ; color
        mov     al, es:[di]
        shr     dx, 1
        jnc     @F
        and     ax, 0F00Fh
        jmp     SHORT   plotsit
@@:     shl     al, 1
        shl     al, 1
        shl     al, 1
        shl     al, 1
        and     ax, 0FF0h
plotsit:
        or      ah, al
        mov     dx, 03DAh
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
        add     di, dx
        or      di, 0001h
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
        add     di, dx
        or      di, 0001h
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
        add     di, dx
        or      di, 0001h
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
        add     di, dx
        or      di, 0001h
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
vlins:  mov     bh, es:[di]
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
