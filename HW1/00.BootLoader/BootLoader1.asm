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


;==================== calculate week ======================
	; add year days / year in bx (2019)
    sub bx, 1		
    call GET_LEAP_CNT       ; leap cnt from 1 to (year-1)
    mov word[LEAPCNT], cx
    mov di, cx
    
	add bx, 1
    call GET_LEAP_CNT       ; leap cnt from 1 to (year)
    mov word[LEAPCNT2], cx

    sub di, 460             ; leap cnt from 1900 to (year-1)

    sub bx, 1900            ; year cnt ?
    imul bx, bx, 0x16D      ; year cnt*365

    add bx, di              ; year cnt*365 + leap cnt

    ; add month days
	xor cx, cx
	mov al, byte[DATEMESSAGE + 3]
	sub al, 0x30
	mov di, 10
	mul di
	mov cl, byte[DATEMESSAGE + 4]
	sub cl, 0x30
	add cl, al			    ; month in cl

    mov si, cx
    dec cx
    shl cx, 1

    add bx, [monthsum+ecx]

    mov ax, word[LEAPCNT2]
    mov cx, word[LEAPCNT]     	   ; year년과 (year-1)년 윤년수 비교
    sub ax, cx             
    test ax, ax

    jz .NOTLEAPYEAR
    cmp si, 3
    js .NOTLEAPYEAR
    inc bx 

.NOTLEAPYEAR:

	; add days
    xor cx, cx
	mov al, byte[DATEMESSAGE + 0]
	sub al, 0x30
	mov di, 10
	mul di
	mov cl, byte[DATEMESSAGE + 1]
	sub cl, 0x30
	add cl, al				; date in cl

    add bx, cx
    xor dx, dx
    mov ax, bx 

    mov cx, 7
    div cx                  ; remainder(%7) in dx


    ; print week string
    mov ax, dx
    mov bx, 3
    mul bx
    mov di, ax
    
    mov cl, byte[WEEK+di]
    mov byte [ es: 210 ], cl
    inc di
    mov cl, byte[WEEK+di]
    mov byte [ es: 212 ], cl
    inc di
    mov cl, byte[WEEK+di]
    mov byte [ es: 214 ], cl

;=========================================================

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

;====================================
GET_LEAP_CNT:
	;bx에 년도 들어옴 / cx에 윤년수 구한거 저장
    mov cx, bx
    shr cx, 2

    mov ax, bx
    mov si, 100
    xor dx, dx
    div si
    sub cx, ax

    mov ax, bx
    mov si, 400
    xor dx, dx
    div si
    add cx, ax 
    ret

;===================================


DATEMESSAGE:		db '00/00/0000', 0	
DISKERRORMESSAGE:	db 'DISK Error', 0

LEAPCNT:                dw  0x00
LEAPCNT2:               dw  0x00
monthsum DW 0,31,59,90,120,151,181,212,243,273,304,334
WEEK:    db  'SUNMONTUEWEDTHUFRISAT', 0

times 510 - ( $ - $$ ) db 0x00

db 0x55
db 0xAA