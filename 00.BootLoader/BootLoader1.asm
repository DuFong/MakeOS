[ORG 0x00]
[BITS 16]

SECTION .text

    
jmp 0x07C0:START

START:
    mov ax, 0x07C0
    mov ds, ax
    mov ax, 0xB800
    mov es, ax

     ;Set Stack
    mov ax, 0x0000
	mov ss, ax
	mov sp, 0xFFFF
	mov bp, 0xFFFF
	
    mov si,    0

	.SCREENCLEARLOOP:
	mov byte [ es: si ], 0
    mov byte [ es: si + 1 ], 0x0A

    add si, 2
    cmp si, 80 * 25 * 2

    jl .SCREENCLEARLOOP

	; Print date of the week
PRINTDAY:
	;example code
	mov di, 210
	mov byte[es:di], 0x39



RESETDISK:
	; BIOS Intterupt
	mov ax, 0
	mov dl, 0
	int 0x13
	;jc HANDLEDISKERROR

	; 0x1000 = address of BootLoader2
	mov si, 0x1000
	mov es, si
	mov bx, 0x0000

	; set disk location of BootLoader2
    mov ah, 0x02
	mov al, 0x1
	mov ch, 0x00	; TRACK NUMBER
	mov cl, 0x02	; SECTOR NUMBER
	mov dh, 0x00	; HEAD NUMBER
	mov dl, 0x00
	int 0x13
	;jc HANDLEDISKERROR

    jmp 0x1000:0x0000


HANDLEDISKERROR:
	mov si, 0
	mov di, 360
	
	.MESSAGELOOP:
	mov cl, byte [ DISKERRORMESSAGE + si ]
	cmp cl, 0
	jmp .MESSAGEEND

	mov byte [ es: di ], cl
	add si, 1
	add di, 2

	jmp .MESSAGELOOP
	.MESSAGEEND:
	jmp $



DISKERRORMESSAGE:	 db 'DISK Error',0

times 510 - ( $ - $$ ) db 0x00

db 0x55
db 0xAA



