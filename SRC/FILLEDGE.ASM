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
_TEXT   SEGMENT
	ASSUME	CS: _TEXT

times   =       5                       ; Must be factor of 80 and 50

    PUBLIC  _setstartaddr
_setstartaddr PROC NEAR
        push    bp
        mov     bp, sp
        mov     bx, WORD PTR [bp+4]      ;startaddr
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
_setstartaddr ENDP

    PUBLIC  _filledgeh
_filledgeh  PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        mov     ax, 0B800H
        mov     es, ax
        mov     di, WORD PTR [bp+4]      ;pixaddr
        mov     si, WORD PTR [bp+6]      ;edgeaddr
        mov     cx, 80/times
cpyedgeh:
        REPT    times
        lodsw
        stosb
        inc     di
        mov     al, ah
        stosb
        inc     di
        ENDM
        loop    cpyedgeh
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_filledgeh  ENDP

        PUBLIC  _filledgev
_filledgev  PROC NEAR
        push    bp
        mov     bp, sp
        push    di
        push    si
        mov     ax, 0B800H
        mov     es, ax
        mov     di, WORD PTR [bp+4]      ;pixaddr
        mov     si, WORD PTR [bp+6]      ;edgeaddr
        mov     cx, 50/times
cpyedgev:
        REPT    times
        lodsw
        mov     es:[di], al
        add     di, 160
        mov     es:[di], ah
        add     di, 160
        ENDM
        loop    cpyedgev
        pop     si
        pop     di
        mov     sp, bp
        pop     bp
        ret
_filledgev  ENDP
_TEXT	ENDS
END
