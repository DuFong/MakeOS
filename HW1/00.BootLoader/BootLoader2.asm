[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x1000:START    ; cs(segment register) : 0x1000

; MINT64 OS setting
TOTALSECTORCOUNT: dw 2


; CODE SECTOR
START:
    mov ax, cs
    mov ds, ax
    mov ax, 0xB800
    mov es, ax

    mov si, 0
    mov di, 0

	; print Start Message
	push MESSAGE1
	push 0
	push 0
	call PRINTMESSAGE
	add sp, 6

	push MESSAGE2
	push 1
	push 0
	call PRINTMESSAGE
	add sp, 6

	; print OS Image Loading
	push IMAGELOADINGMESSAGE
	push 2
	push 0
	call PRINTMESSAGE
	add sp, 6

RESETDISK:
	; BIOS Intterupt
	mov ax, 0
	mov dl, 0
	int 0x13
	jc HANDLEDISKERROR

	; read sectors from disk
	mov si, 0x1020
	mov es, si
	mov bx, 0x0000

	mov di, word[ TOTALSECTORCOUNT ]

READDATA:
	; check read all sectors
	cmp di, 0
	je READEND
	sub di, 0x1

	; call BIOS read function
	mov ah, 0x02                    ; BIOS service number 2(read sector)
    mov al, 0x1                     ; number of sector to read
	mov ch, byte[ TRACKNUMBER ]
	mov cl, byte[ SECTORNUMBER ]
	mov dh, byte[ HEADNUMBER ]
	mov dl, 0x00                    ; drive number 0(floppy)
    int 0x13                        ; interrupt service
	jc HANDLEDISKERROR
	
	; calculate address, track, head, sector
    add si, 0x0020                  ; after read 512(0x200)bytes, change into segment register value
    mov es, si

	; to 18th sector
	mov al, byte[ SECTORNUMBER ]
	add al, 0x01
	mov byte[ SECTORNUMBER ], al
	cmp al, 19
	jl READDATA

	; last sector -> head toggle(0->1, 1->0) and set sector number 1
	xor byte[ HEADNUMBER ], 0x01
	mov byte[ SECTORNUMBER ], 0x01

	; read both head -> increase track number
	cmp byte[ HEADNUMBER ], 0x00
	jne READDATA
	add byte[ TRACKNUMBER ], 0x01
	jmp READDATA

READEND:
	push LOADINGCOMPLETEMESSAGE
	push 2
	push 20
	call PRINTMESSAGE
	add sp, 6

	; virtual OS image execute
	jmp 0x1020:0x0000


PRINTMESSAGE:
	push bp
	mov bp, sp						; access parameter by bp(base pointer register)

	push es
	push si
	push di
	push ax
	push cx
	
	; set segment register video mode address
	mov ax, 0xB800
	mov es, ax

	; calculate address of video memory by X,Y
    ; by Y
	mov ax, word[ bp + 6 ]
	mov si, 160
	mul si
	mov di, ax

	; by X
	mov ax, word[ bp + 4 ]
	mov si, 2
	mul si
	add di, ax

	; string address
	mov si, word[ bp + 8 ]

	.MESSAGELOOP:
		mov cl, byte [ si ]
		cmp cl, 0
		je .MESSAGEEND

		mov byte [ es: di ], cl
		add si, 1
		add di, 2

		jmp .MESSAGELOOP

	.MESSAGEEND:
		pop cx
		pop ax
		pop di
		pop si
		pop es
		pop bp
		ret

HANDLEDISKERROR:
	push DISKERRORMESSAGE
	push 2
	push 20
	call PRINTMESSAGE

	jmp $

MESSAGE1:					db 'MINT64 OS Boot Loader Start~!!', 0
MESSAGE2:					db 'Current Date:' , 0
IMAGELOADINGMESSAGE: 		db 'OS Image Loading...', 0
DISKERRORMESSAGE:	 		db 'DISK Error',0
LOADINGCOMPLETEMESSAGE: 	db 'Complete~!!', 0

SECTORNUMBER:				db 0x03
HEADNUMBER: 				db 0x00
TRACKNUMBER: 				db 0x00

times 510 - ( $ - $$ )    db    0x00

db 0x55
db 0xAA
