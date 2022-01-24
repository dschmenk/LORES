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

EXTRN   _edgeH:	BYTE
EXTRN   _edgeV:	BYTE

_TEXT   SEGMENT
;        ASSUME  CS: _TEXT, DS: DGROUP

unrollcpybuf    = 1

        PUBLIC  _setStartAddr
_setStartAddr   PROC NEAR
        push    bp
        mov     bp, sp
        mov     bx, WORD PTR [bp+4]     ;startaddr
        mov     sp, bp
        pop     bp
; Wait for beginning of VBlank
        mov     dx, 03DAh               ; CGA status
waitvbl:
        in      al, dx
        test    al, 08h
        jnz     waitvbl
waitact:
        in      al, dx
        test    al, 08h
        jz      waitact
; Update CGA start address
        mov     dx, 03D4h
        mov     al, 12
        mov     ah, bh
        out     dx, ax
        inc     al
        mov     ah, bl
        out     dx, ax
        ret
_setStartAddr   ENDP

        PUBLIC  __cpyEdgeH
__cpyEdgeH      PROC NEAR
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
__cpyEdgeH      ENDP

        PUBLIC  __cpyEdgeV
__cpyEdgeV      PROC NEAR
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
__cpyEdgeV      ENDP

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
cpybufv:
        mov     cx, WORD PTR [bp+6]     ; width/2
        push    si
        push    di
        shr     cx, 1
        jz      cpybufnxt
cpybufh:
        REPT    unrollcpybuf
        movsb
        inc     di
        movsb
        inc     di
        dec     cx
        jz      SHORT cpybufnxt
        ENDM
        movsb
        inc     di
        movsb
        inc     di
        loop    cpybufh
cpybufnxt:
        jae     SHORT @F
        movsb
@@:     pop     di
        add     di, 160
        pop     si
        add     si, WORD PTR [bp+10]
        dec     WORD PTR [bp+8]
        jnz    cpybufv
        pop     ds
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
        mov     dx, 03DAh
        mov     ax, 0B800H
        mov     es, ax
        mov     di, WORD PTR [bp+4]     ; pixaddr
        lds     si, DWORD PTR [bp+12]   ; pixbuf
        ASSUME  DS: NOTHING
cpybufsv:
        mov     cx, WORD PTR [bp+6]     ; width/2
        push    si
        push    di
        shr     cx, 1
        jae     SHORT cpybufsh2
@@:     in      al, dx
        shr     al, 1
        jae     @B
        movsb
        jcxz    cpybufsnxt
        inc     di
cpybufsh2:
        shr     cx, 1
        jae     SHORT cpybufsh4
@@:     in      al, dx
        shr     al, 1
        jae     @B
        movsb
        inc     di
        movsb
        jcxz    cpybufsnxt
        inc     di
cpybufsh4:
        REPT    unrollcpybuf
@@:     in      al, dx
        shr     al, 1
        jae     @B
        movsb
        inc     di
        movsb
        inc     di
        movsb
        inc     di
        movsb
        inc     di
        dec     cx
        jz      SHORT cpybufsnxt
        ENDM
@@:     in      al, dx
        shr     al, 1
        jae     @B
        movsb
        inc     di
        movsb
        inc     di
        movsb
        inc     di
        movsb
        inc     di
        loop    cpybufsh4
cpybufsnxt:
        pop     di
        add     di, 160
        pop     si
        add     si, WORD PTR [bp+10]
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
