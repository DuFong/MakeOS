#include "PIC.h"

// PIC 초기화
void kInitializePIC(){
    /* 마스터 PIC 컨트롤러 초기화 */
    // ICW1(port 0x20), IC4 bit(bit 0) = 1
    kOutPortByte(PIC_MASTER_PORT1, 0x11);

    // ICW2(port 0x21), Interrupt vector(0x20)
    kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR);

    // ICW3(port 0x21), 슬레이브 PIC 컨트롤러가 연결 위치(비트로 표현)
    // 마스터 PIC 컨트롤러의 2번 핀에 연결되어 있으므로 0x04(bit 2)로 설정
    kOutPortByte(PIC_MASTER_PORT2, 0x04);

    // ICW4(port 0x21), uPM bit(bit 0) = 1
    kOutPortByte(PIC_MASTER_PORT2, 0x01);

    /* 슬레이브 PIC 컨트롤러 초기화 */
    // ICW1(port 0xA0), IC4 bit(bit 0) = 1
    kOutPortByte(PIC_SLAVE_PORT1, 0x11);

    // ICW2(port 0xA1), Interrupt vector(0x20 + 8)
    kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8);

    // ICW3(port 0xA1), 마스터 PIC 컨트롤러에 연결된 위치(정수로 표현)
    // 마스터 PIC 컨트롤러의 2번 핀에 연결되어 있으므로 0x02로 설정
    kOutPortByte(PIC_SLAVE_PORT2, 0x02);

    // ICM4(port 0xA1), uPM bit(bit 0) = 1
    kOutPortByte(PIC_SLAVE_PORT2, 0x01);
}

// 인터럽트를 마스크하여 해당 인터럽트가 발생하지 않게 처리
void kMaskPICInterrupt(WORD wIRQBitmask){
    // 마스터 PIC 컨트롤러에 IMR 설정
    // OCW1(port 0x21), IRQ0 ~ IRQ7
    kOutPortByte(PIC_MASTER_PORT2, (BYTE) wIRQBitmask);

    // 슬레이브 PIC 컨트롤러에 IMR 설정
    // OCW1(port 0xA1), IRQ8 ~ IRQ15
    kOutPortByte(PIC_SLAVE_PORT2, (BYTE) (wIRQBitmask >> 8));
}

// 인터럽트 처리가 완료되었음을 전송(EOI)
// Master PIC : only Master
// Slave PIC : both
void kSendEOIToPIC(int iIRQNumber){
    // 마스터 PIC 컨트롤러에 EOI 전송
    // OCW2(port 0x20), EOI bit(bit 5) = 1
    kOutPortByte(PIC_MASTER_PORT1, 0x20);

    // 슬레이브 PIC 컨트롤러의 인터럽트인 경우 슬레이브에게도 전송
    if(iIRQNumber >= 8){
        // OCW2(port 0xA0), EOI bit(bit 5) = 1
        kOutPortByte(PIC_SLAVE_PORT1, 0x20);
    }
}