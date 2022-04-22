	TITLE   keyboard.asm
	NAME    keyboard
_TEXT   SEGMENT  WORD PUBLIC 'CODE'
_TEXT   ENDS
_DATA   SEGMENT  WORD PUBLIC 'DATA'
_DATA   ENDS
CONST   SEGMENT  WORD PUBLIC 'CONST'
CONST   ENDS
_BSS    SEGMENT  WORD PUBLIC 'BSS'
_BSS    ENDS
DGROUP  GROUP	CONST, _BSS, _DATA
_TEXT   SEGMENT
        ASSUME	CS: _TEXT, DS: DGROUP, SS: DGROUP

oldKeyboardISR  DD  00h
isPrevCodeExt	DB  00h
keyStates       DB  256 DUP(00h)

        PUBLIC	_keyboardGetKey
_keyboardGetKey	PROC NEAR
        push    bp
        mov	    bp, sp
;	scanCode = 4
        xor     ax,ax
        mov	    bx, WORD PTR [bp+4]	;scanCode
        cmp	    bh, 0E0h
	    jne	    @F
        xor     bh, bh
        add     bx, 127
@@:     mov     al, BYTE PTR cs:keyStates[bx]
        pop     bp
        ret
_keyboardGetKey	ENDP

	   PUBLIC  _keyboardClearKeys
_keyboardClearKeys	PROC NEAR
        push	di
        mov     cx, 256/2
        mov     di, OFFSET _TEXT:keyStates
        mov     ax, cs
        mov     es, ax
        xor     ax, ax
    rep stosw
        pop     di
        ret
_keyboardClearKeys	ENDP

        PUBLIC  _keyboardInstallDriver
_keyboardInstallDriver	PROC NEAR
        mov     ax, WORD PTR cs:oldKeyboardISR+0
        or      ax, WORD PTR cs:oldKeyboardISR+2
        jnz     @F
        push    si
        push    di
        push    ds                      ; Save current keyboard ISR
        xor     ax, ax
        mov     ds, ax
        ASSUME  DS: NOTHING
        mov     si, 024h                ; IRQ 09
        lodsw
        mov     WORD PTR cs:oldKeyboardISR+0, ax
        lodsw
        mov     WORD PTR cs:oldKeyboardISR+2, ax
        pop     ds
        ASSUME  DS: DGROUP
        xor     ax, ax                  ; Install our ISR handler
        mov     es, ax
        mov     di, 024h                ; IRQ 09
        mov     ax, OFFSET _TEXT:keyboardISR
        stosw
        mov     ax, cs
        stosw
        call    _keyboardClearKeys
        pop     di
        pop     si
@@:     ret
_keyboardInstallDriver	ENDP

        PUBLIC  _keyboardUninstallDriver
_keyboardUninstallDriver    PROC NEAR
        mov     ax, WORD PTR cs:oldKeyboardISR+0
        or      ax, WORD PTR cs:oldKeyboardISR+2
        jz      @F
        push    si
        push    di
        xor     ax, ax                  ; Re-install old ISR handler
        mov     es, ax
        mov     di, 024h                ; IRQ 09
        mov     ax, WORD PTR cs:oldKeyboardISR+0
        stosw
        mov     ax, WORD PTR cs:oldKeyboardISR+2
        stosw
        xor     ax, ax
        mov     WORD PTR cs:oldKeyboardISR+0, ax
        mov     WORD PTR cs:oldKeyboardISR+2, ax
        pop     di
        pop     si
@@:     ret
_keyboardUninstallDriver    ENDP

        ASSUME  DS: NOTHING, SS: NOTHING
keyboardISR     PROC FAR
        push	ax
        push    bx
        xor     bx, bx
        in      al, 061h                ; PPI port B
        mov     ah, al
        in      al, 060h                ; KB output buffer
        mov     bl, al
        mov     al, ah
        or      al, 128
        out     061h, al                ; PPI port B
        mov     al, ah
        out     061h, al                ; PPI port B
        cmp     bl, 0E0h
        jne     @F
; Set up for extended scan code
        mov     BYTE PTR cs:isPrevCodeExt, 1
        jmp     SHORT kbExitISR
@@:     cmp     BYTE PTR cs:isPrevCodeExt, 0
        je      kbKey
; Extended key
        mov     BYTE PTR cs:isPrevCodeExt, 0
        test    bl, 128
        je      kbExtKeyPress
        and     bl, 127
        mov     BYTE PTR cs:keyStates[bx+127], bh
        jmp     SHORT kbExitISR
kbExtKeyPress:
        mov     BYTE PTR cs:keyStates[bx+127], 1
        jmp     SHORT kbExitISR
; Regular key
kbKey:
        test    bl, 128
        jz      kbKeyPress
        and     bl, 127
        mov     BYTE PTR cs:keyStates[bx], bh
        jmp     SHORT kbExitISR
kbKeyPress:
        mov     BYTE PTR cs:keyStates[bx], 1
kbExitISR:
        mov     al, 020h
        out     020h, al
        pop     bx
        pop     ax
        iret
keyboardISR     ENDP

_TEXT   ENDS
END
