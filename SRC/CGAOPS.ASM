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

cpybufsel       DW  cpybufh8, cpybufh1, cpybufh2, cpybufh3
                DW  cpybufh4, cpybufh5, cpybufh6, cpybufh7

_DATA   ENDS


_TEXT   SEGMENT

unrollcpybuf    = 1

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
        jae     @B
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
; pixaddr       = 4
; width/2       = 6
; height        = 8
; span          = 10
; bufptr        = 12
        mov     ax, 0B800H
        mov     es, ax
        mov     di, WORD PTR [bp+4]     ; pixaddr
        lds     si, DWORD PTR [bp+12]   ; pixbuf
        ASSUME  DS: NOTHING
        mov     dx, WORD PTR [bp+8]     ; height
        mov     cx, WORD PTR [bp+6]     ; width/2
        sub     WORD PTR [bp+10], cx    ; span -= width/2
        mov     bx, cx
        and     bx, 07h
        shl     bx, 1
        mov     bx, cpybufsel[bx]
cpybufv:
        add     cx, 7                   ; Round up iteration count
        shr     cx, 1
        shr     cx, 1
        shr     cx, 1
        jmp     bx
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
        loop    cpybufh8
cpybufnxt:
        add     di, 160
        mov     cx, WORD PTR [bp+6]     ; width/2
        sub     di, cx
        sub     di, cx
        add     si, WORD PTR [bp+10]
        dec     dx
        jnz     cpybufv
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
; pixaddr       = 4
; width/2       = 6
; height        = 8
; span          = 10
; bufptr        = 12
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
        jae     SHORT cpybufsh
@@:     in      al, dx
        shr     al, 1
        jae     @B
        movsb
        inc     di
        jcxz    cpybufsnxt
cpybufsh:
@@:     in      al, dx
        shr     al, 1
        jae     @B
        movsb
        inc     di
@@:     in      al, dx
        shr     al, 1
        jae     @B
        movsb
        inc     di
        loop    cpybufsh
cpybufsnxt:
        add     di, 160
        add     si, WORD PTR [bp+10]
        sub     di, bx
        sub     di, bx
        dec     WORD PTR [bp+8]
        jnz    cpybufsv
        pop     ds
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
__cpyBufSnow    ENDP

_TEXT	ENDS
END
