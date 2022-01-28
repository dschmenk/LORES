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

        PUBLIC	_frameCount
        PUBLIC	_rasterTimer
        PUBLIC	_enableRasterTimer
        PUBLIC  _disableRasterTimer
        PUBLIC  _statusRasterTimer

_DATA   SEGMENT
_frameCount     DW  0
_rasterTimer    DB  0
_DATA   ENDS

_TEXT   SEGMENT
        ASSUME	CS: _TEXT

chainPIT        DD  0
chainCount      DB  3

_enableRasterTimer  PROC NEAR
        mov     ax, WORD PTR cs:chainPIT+2
        or      ax, ax
        jz      @F
        ret                             ; Already enabled
@@:     push    bp
        mov     bp, sp
        push    si
        push    di
        mov     cx, [bp+4]              ; Scanline to synchronize on
        cli
        mov     dx, 03DAh               ; CGA status
waitact:                                ; Wait for VBlank
        in      al, dx
        test    al, 08h
        jz      waitact                 ; Wait if currently in VBlank
waitvbl:
        in      al, dx
        test    al, 08h
        jnz     waitvbl                 ; Wait until VBlank
waitscan:                               ; Count number of scanlines before reloading PIT to synchronize
@@:     in      al, dx                  ; Wait while display inactive
        shr     al, 1
        jb      @B
@@:     in      al, dx                  ; Wait while display active
        shr     al, 1
        jae     @B
        loop    waitscan
        mov     al, 036h                ; Program PIT to count 19912
        out     043h, al                ; Timer 0, LSB, MSB, mode 3
        mov     al, 0C8h                ; 59.94 Hz
        out     040h, al
        mov     al, 04Dh
        out     040h, al
        push    ds                      ; Save current PIT ISR
        xor     ax, ax
        mov     ds, ax
        ASSUME  DS: NOTHING
        mov     si, 020h                ; IRQ 08
        lodsw
        mov     WORD PTR cs:chainPIT+0, ax
        lodsw
        mov     WORD PTR cs:chainPIT+2, ax
        pop     ds
        ASSUME  DS: DGROUP
        xor     ax, ax                  ; Install our ISR handler
        mov     es, ax
        mov     di, 020h                ; IRQ 08
        mov     ax, OFFSET _TEXT:timerISR
        stosw
        mov     ax, cs
        stosw
        mov     BYTE PTR cs:chainCount, 3 ; Set chain count - chain to old PIT handler every third IRQ
        sti
        xor     ax, ax
        inc     ax                      ; Return success
        pop     di
        pop     si
        mov     sp, bp
        pop     bp
        ret
_enableRasterTimer  ENDP

_statusRasterTimer  PROC NEAR
        xor     ah, ah
        mov     al, _rasterTimer
        ret
_statusRasterTimer  ENDP

_disableRasterTimer PROC NEAR
        mov     ax, WORD PTR cs:chainPIT+2
        or      ax, ax
        jnz     @F
        ret
@@:     push    di
        cli
        mov     al, 036h                ; Program PIT to count 65536
        out     043h, al                ; Timer 0, LSB, MSB, mode 3
        xor     al, al                  ; 18.2 Hz
        out     040h, al
        out     040h, al
        xor     dx, dx                  ; Re-install previous ISR handler
        mov     es, dx
        mov     di, 020h                ; IRQ 08
        mov     ax, WORD PTR cs:chainPIT+0
        stosw
        mov     ax, WORD PTR cs:chainPIT+2
        stosw
        mov     WORD PTR cs:chainPIT+0, dx
        mov     WORD PTR cs:chainPIT+2, dx
        sti
        pop     di
        xor     ax, ax                  ; Return success
        inc     ax
        ret
_disableRasterTimer ENDP

timerISR        PROC FAR
        ASSUME  DS: NOTHING
        push    ax
        push    ds
        mov     ax, DGROUP
        mov     ds, ax
        ASSUME  DS: DGROUP
        mov     BYTE PTR _rasterTimer, 0
        inc     WORD PTR _frameCount
        pop     ds
        ASSUME  DS: NOTHING
        dec     BYTE PTR cs:chainCount
        jz      SHORT @F
        mov     al, 020h
        out     020h, al                ; Send EOI to 8259
        pop     ax
        iret
@@:     pop     ax
        mov     BYTE PTR cs:chainCount, 3
        jmp     DWORD PTR cs:[chainPIT]
timerISR        ENDP
_TEXT   ENDS
END
