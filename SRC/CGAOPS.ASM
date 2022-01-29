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

EXTRN   _edgeH: BYTE
EXTRN   _edgeV: BYTE
EXTRN   _rasterTimer: BYTE

_DATA   SEGMENT

cpybufsel       DW  cpybufh32, cpybufh1,  cpybufh2,  cpybufh3
                DW  cpybufh4,  cpybufh5,  cpybufh6,  cpybufh7
                DW  cpybufh8,  cpybufh9,  cpybufh10, cpybufh11
                DW  cpybufh12, cpybufh13, cpybufh14, cpybufh15
                DW  cpybufh16, cpybufh17, cpybufh18, cpybufh19
                DW  cpybufh20, cpybufh21, cpybufh22, cpybufh23
                DW  cpybufh24, cpybufh25, cpybufh26, cpybufh27
                DW  cpybufh28, cpybufh29, cpybufh30, cpybufh31

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
        loop    SHORT @F
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
        mov     di, WORD PTR [bp+4]     ;pixaddr
        mov     si, OFFSET DGROUP:_edgeV
cpyedgev:
        REPT    100
        movsb
        add     di, 159
        ENDM
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_cpyEdgeV       ENDP

        PUBLIC  __cpyBuf
__cpyBuf        PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
; pixaddr       = bp+4
; width/2       = bp+6
; height        = bp+8
; span          = bp+10
; buf           = bp+12
        mov     cx, WORD PTR [bp+6]     ; width/2
        mov     bx, cx
        mov     dl, cl                  ; width/2 must be less than 80
        mov     dh, BYTE PTR [bp+8]     ; height must be 100 or less
        and     bx, 01Fh
        shl     bx, 1
        mov     bx, cpybufsel[bx]
        mov     ax, 0B800H
        mov     es, ax
        mov     di, WORD PTR [bp+4]     ; pixaddr
        lds     si, DWORD PTR [bp+12]   ; buf
        ASSUME  DS: NOTHING
        push    bp                      ; This is bad. Very bad
        mov     bp, WORD PTR [bp+10]    ; span
        sub     bp, cx                  ; span -= width/2
cpybufv:
        add     cx, 31                  ; Round up iteration count
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        jmp     bx
cpybufh32:
        movsb
        inc     di
cpybufh31:
        movsb
        inc     di
cpybufh30:
        movsb
        inc     di
cpybufh29:
        movsb
        inc     di
cpybufh28:
        movsb
        inc     di
cpybufh27:
        movsb
        inc     di
cpybufh26:
        movsb
        inc     di
cpybufh25:
        movsb
        inc     di
cpybufh24:
        movsb
        inc     di
cpybufh23:
        movsb
        inc     di
cpybufh22:
        movsb
        inc     di
cpybufh21:
        movsb
        inc     di
cpybufh20:
        movsb
        inc     di
cpybufh19:
        movsb
        inc     di
cpybufh18:
        movsb
        inc     di
cpybufh17:
        movsb
        inc     di
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
        loop    cpybufh32
cpybufnxt:
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
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
__cpyBuf        ENDP

        PUBLIC  __cpyBufSnow
__cpyBufSnow    PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        push    ds
; pixaddr       = bp+4
; width/2       = bp+6
; height        = bp+8
; span          = bp+10
; bufptr        = bp+12
        mov     ax, 0B800H
        mov     es, ax
        mov     di, WORD PTR [bp+4]     ; pixaddr
        lds     si, DWORD PTR [bp+12]   ; pixbuf
        ASSUME  DS: NOTHING
        mov     bx, WORD PTR [bp+6]     ; width/2
        sub     WORD PTR [bp+10], bx    ; span -= width/2
        mov     dx, 03DAh
cpybufsv:
        mov     cx, bx
        shr     cx, 1
        jnc     SHORT cpybufsh
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
        add     si, WORD PTR [bp+10]    ; span
        sub     di, bx
        sub     di, bx
        dec     WORD PTR [bp+8]         ; height
        jnz     cpybufsv
        pop     ds
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
__cpyBufSnow    ENDP

_TEXT	ENDS
END
