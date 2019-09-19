[ORG 0x00]
[BITS 16]

SECTION .text

    ; cs(segment register) : 0x1000

START:
    mov ax, cs
    mov ds, ax
    mov ax, 0xB800
    mov es, ax

    mov di, 160
    mov cx, 0x39

    mov byte[es:di], cl

    jmp $

times 512 - ( $ - $$ ) db 0x00


