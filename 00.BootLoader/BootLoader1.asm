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
    mov byte [ es: si + 1 ], 0x0A	; black background, green text

    add si, 2
    cmp si, 80 * 25 * 2		; screen size: 80letters * 25lines

    jl .SCREENCLEARLOOP

; Print date of the week
GETTODAY:
	; bios interrupt service
    mov ah, 0x04
    int 0x1A

    ; date
    mov al, dl
    shr al, 4
    add al, 0x30
    mov [DATEMESSAGE+0], al
    mov al, dl
    and al, 0x0F
    add al, 0x30
    mov [DATEMESSAGE + 1], al

    ; month
    mov al, dh
    shr al, 4
    add al, 0x30
    mov [DATEMESSAGE + 3], al
    mov al, dh
    and al, 0x0F
    add al, 0x30
    mov [DATEMESSAGE + 4], al

    xor ax, ax

    ; year
    mov al, ch
    shr al, 4                   ; 2
    mov si, 1000
    mul si                      ; ax = 2 x 1000
    mov bx, ax                  ; bx = 2000
    div si
    add al, 0x30
    mov [DATEMESSAGE + 6], al   
    mov al, ch
    and al, 0x0F                ; 0
    mov si, 100
    mul si                      ; ax = 0 x 100
    add bx, ax                  ; bx = 2000 + 000
    div si
    add al, 0x30
    mov [DATEMESSAGE + 7], al   
    mov al, cl
    shr al, 4                   ; 1
    mov si, 10
    mul si                      ; ax = 1 x 10
    add bx, ax                  ; bx = 2000 + 000 + 10
    div si
    add al, 0x30
    mov [DATEMESSAGE + 8], al   
    mov al, cl
    and al, 0x0F                ; 9
    add bx, ax                  ; bx = 2019
    add al, 0x30
    mov [DATEMESSAGE + 9], al

	;;;;;;;;;;;;;;;; test for printing
	; sub bx, 1970
	; mov di, 210
	; mov byte [es:di], bl
	;;;;;;;;;;;;;;;;;;;;;;;;

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; add code
	shr bx, 7
	add bx, 82
	mov di, 210
	mov byte [es:di], bl

    mov ax, bx                  ; bx = 2019
    sub ax, 1900                ; ax = 2019 - 1900 = 119
    mov si, 365
    mul si                      ; ax = 119 x 365	

; print today date
	xor si, si
	mov di, 188
PRINTDAY:
	mov cl, byte [DATEMESSAGE + si]
	test cl, cl
	je RESETDISK

	mov byte [ es: di ], cl
	add si, 1
	add di, 2

	jmp PRINTDAY


RESETDISK:
	; BIOS Intterupt
	mov ax, 0
	mov dl, 0
	int 0x13
	jc HANDLEDISKERROR

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
	jc HANDLEDISKERROR

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



DATEMESSAGE:		db '00/00/0000', 0	
DISKERRORMESSAGE:	db 'DISK Error', 0

times 510 - ( $ - $$ ) db 0x00

db 0x55
db 0xAA