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

        PUBLIC	_rasterActive
        PUBLIC	_frameCount
        PUBLIC	_enableRasterTimer
        PUBLIC  _disableRasterTimer
        PUBLIC  _rasterStatus

_DATA   SEGMENT
_rasterActive   DB  0
_frameCount     DW  0
_DATA   ENDS

_TEXT   SEGMENT
        ASSUME	CS: _TEXT

chainPIT        DD  0

_enableRasterTimer  PROC NEAR
        mov     ax, WORD PTR cs:chainPIT+2
        or      ax, ax
        jz      @F
        ret
@@:     push    bp
        mov     bp, sp
        push    si
        push    di
        mov     cx, [bp+4]              ; Scanline to synchronize on
; Disable interrupts
        cli
; Wait for end of VBlank
        mov     dx, 03DAh               ; CGA status
waitact:
        in      al, dx
        test    al, 08h
        jz      waitact
waitvbl:
        in      al, dx
        test    al, 08h
        jnz     waitvbl
; Count number of scanlines before reloading PIT to synchronize
waitscans:
@@:     in      al, dx
        shr     al, 1
        jb      @B
@@:     in      al, dx
        shr     al, 1
        jae     @B
        loop    waitscans
; Program PIT to count 19912
        mov     al, 036h            ; Timer 0, LSB, MSB, mode 3
        out     043h, al
        mov     al, 0C8h            ; 59.94 Hz
        out     040h, al
        mov     al, 04Dh
        out     040h, al
; Save current PIT ISR
        push    ds
        xor     ax, ax
        mov     ds, ax
        ASSUME  DS: NOTHING
        mov     si, 020h            ; IRQ 08
        lodsw
        mov     WORD PTR cs:chainPIT+0, ax
        lodsw
        mov     WORD PTR cs:chainPIT+2, ax
        pop     ds
        ASSUME  DS: DGROUP
; Install our ISR handler
        xor     ax, ax
        mov     es, ax
        mov     di, 020h            ; IRQ 08
        mov     ax, OFFSET _TEXT:timerISR
        stosw
        mov     ax, cs
        stosw
; Enable interrupts
        sti
        xor     ax, ax
        inc     ax                  ; Return success
        pop     di
        pop     si
        mov     sp, bp
        pop     bp
        ret
_enableRasterTimer  ENDP

_rasterStatus       PROC NEAR
        xor     ah, ah
        mov     al, _rasterActive
        ret
_rasterStatus       ENDP

_disableRasterTimer PROC NEAR
        mov     ax, WORD PTR cs:chainPIT+2
        or      ax, ax
        jnz     @F
        ret
@@:     push    di
; Disable interrupts
        cli
; Program PIT to count 65536
        mov     al, 036h            ; Timer 0, LSB, MSB, mode 3
        out     043h, al
        xor     al, al              ; 18.2 Hz
        out     040h, al
        out     040h, al
; Re-install previous ISR handler
        xor     dx, dx
        mov     es, dx
        mov     di, 020h            ; IRQ 08
        mov     ax, WORD PTR cs:chainPIT+0
        stosw
        mov     ax, WORD PTR cs:chainPIT+2
        stosw
        mov     WORD PTR cs:chainPIT+0, dx
        mov     WORD PTR cs:chainPIT+2, dx
; Enable interrupts
        sti
; Return success
        pop     di
        xor     ax, ax
        inc     ax
        ret
_disableRasterTimer ENDP

timerISR        PROC FAR
        ASSUME  DS: NOTHING
        push    ds
        push    ax
        mov     ax, DGROUP
        mov     ds, ax
        ASSUME  DS: DGROUP
        mov     BYTE PTR _rasterActive, 0
        inc     WORD PTR _frameCount
        pop     ax
        pop     ds
        ASSUME  DS: NOTHING
        jmp     DWORD PTR cs:[chainPIT]
timerISR        ENDP
_TEXT   ENDS
END
