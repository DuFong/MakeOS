[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte, kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kGetExceptionAddress, kGetPML4BaseAddress

; 포트로부터 1바이트를 읽음
; 인자: 포트번호
kInPortByte:
    push rdx

    mov rdx, rdi    ; RDX 레지스터에 파라미터1(포트 번호)를 저장
    mov rax, 0      ; RAX 레지스터를 초기화
    in al, dx       ; DX 레지스터에 저장된 포트 어드레스에서 한 바이트를 읽어 AL레지스터에 저장

    pop rdx
    ret

; 포트에 1바이트를 씀
; 인자: 포트 번호, 데이터
kOutPortByte:
    push rdx
    push rax

    mov rdx, rdi    ; RDX 레지스터에 파라미터1(포트 번호)를 저장
    mov rax, rsi    ; RAX 레지스터에 파라미터2(데이터)를 저장
    out dx, al      ; DX 레지스터에 저장된 포트 어드레스에 AL 레지스터에 저장된 한 바이트를 씀

    pop rax
    pop rdx
    ret

; GDTR 레지스터에 GDT 테이블을 설정
; PARAM: GDT 테이블의 정보를 저장하는 자료구조의 어드레스
kLoadGDTR:
    lgdt[rdi]       ; 파라미터1(GDTR의 어드레스)를 프로세서에 로드하여 GDT 테이블을 설정
    ret

; TR 레지스터에 TSS 세그먼트 디스크립터 설정
; PARAM: TSS 세그먼트 디스크립터의 오프셋
kLoadTR:
    ltr di          ; 파라미터1(TSS 세그먼트 디스크립터의 오프셋)을 프로세서에 설정하여 TSS 세그먼트 로드
    ret

; IDTR 레지스터에 IDT 테이블을 설정
; PARAM: IDT 테이블의 정보를 저장하는 자료구조의 어드레스
kLoadIDTR:
    lidt[rdi]       ; 파라미터1(IDTR의 어드레스)을 프로세서에 로드하여 IDT 테이블 설정
    ret

; 인터럽트 활성화
kEnableInterrupt:
    sti
    ret

; 인터럽트 비활성화
kDisableInterrupt:
    cli
    ret

; RFLAGS 레지스터 읽기
kReadRFLAGS:
    pushfq
    pop rax
    ret

; cr2 레지스터의 값 반환
kGetExceptionAddress:
    mov rax, cr2
    ret

; cr3 레지스터의 값 반환
kGetPML4BaseAddress:
    mov rax, cr3
    ret