[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x1000:START    ; cs(segment register) : 0x1000

SECTORCOUNT:    dw 0x0000
TOTALSECTORCOUNT    equ 1024

START:
    mov ax, cs
    mov ds, ax
    mov ax, 0xB800
    mov es, ax

    ; code by sector
    %assign i    0
    %rep    TOTALSECTORCOUNT
        %assign i   i + 1

        ; location of current sector -> screen coordinates
        mov ax, 2
        mul word [SECTORCOUNT]
        mov si, ax

        mov byte [es: si + (160 * 3)], '0' + (i % 10)
        add word [SECTORCOUNT], 1

        ; if last sector then infinite loop or move to next sector
        %if i == TOTALSECTORCOUNT
            jmp (0x1000 + i * 0x20): 0x0000
        %else
            jmp (0x1000 + i * 0x20): 0x0000     ; move to the next sector offset
        %endif

        times (512 - ($ - $$) % 512)    db 0x00

%endrep
