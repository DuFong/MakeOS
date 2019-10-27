#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"

// 16진수 표현
char kConvert2Hex(BYTE bDigit){
    if(bDigit < 10){
        return '0' + bDigit;
    }

    return bDigit - 10 + 0x61;
}

// 페이지 폴트 출력
void kPrintPageFault(char* vcBuffer){
    kPrintString(0, 0, "=========================================");
    kPrintString(0, 1, "           Page Fault Occur~!!!!         ");
    kPrintString(0, 2, "           Address: ");
    kPrintString(20, 2, vcBuffer);
    kPrintString(0, 3, "=========================================");
}

// protection fault 출력
void kPrintProtectionFault(char* vcBuffer){
    kPrintString(0, 0, "=========================================");
    kPrintString(0, 1, "        Protection Fault Occur~!!!!      ");
    kPrintString(0, 2, "           Address: ");
    kPrintString(20, 2, vcBuffer);
    kPrintString(0, 3, "=========================================");
}

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode){
    char vcBuffer[3] = {0, };

    // 인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[0] = '0' + iVectorNumber / 10;
    vcBuffer[1] = '0' + iVectorNumber % 10;

    kPrintString(0, 0, "=========================================");
    kPrintString(0, 1, "            Exception Occur~!!!!         ");
    kPrintString(0, 2, "                 Vector: ");
    kPrintString(27, 2, vcBuffer);
    kPrintString(0, 3, "=========================================");

    while(1);
}

void kPageFaultHandler(int iVectorNumber, QWORD qwErrorCode){
    char vcBuffer[9] = {0, };

    // 예외가 발생한 주소
    QWORD qwExceptionAddress = kGetExceptionAddress();
    // 예외가 발생한 주소의 각 자릿수를 구함
    BYTE bAddressDigit[6];
    BYTE bMask = 0xF;
    int i, shift;
    shift = 20;
    for(i = 0; i < 6; i++){
        bAddressDigit[i] = qwExceptionAddress >> shift;
        bAddressDigit[i] &= bMask;
        shift -= 4;
    }

    // 예외가 발생한 주소 출력
    vcBuffer[0] = '0';
    vcBuffer[1] = 'x';
    for(i = 0; i < 6; i++){
        vcBuffer[i + 2] = kConvert2Hex(bAddressDigit[i]);
    }

    // 에러 코드를 통해 protection 위반이 있었는지 확인
    if(qwErrorCode & 1){
        kPrintProtectionFault(vcBuffer);
    }
    else{
        kPrintPageFault(vcBuffer);
    }
}

void kCommonInterruptHandler(int iVectorNumber){
    char vcBuffer[] = "[INT:  , ]";
    static int g_iCommonInterruptCount = 0;

    //===============================================================
    // 인터럽트가 발생했음을 알리고 메시지를 출력
    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[8] = '0' + g_iCommonInterruptCount;
    g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
    kPrintString(70, 0, vcBuffer);
    //===============================================================

    // EOI 전송
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kKeyboardHandler(int iVectorNumber){
    char vcBuffer[] = "[INT:  , ]";
    static int g_iKeyboardInterruptCount = 0;
    BYTE bTemp;

    //===============================================================
    // 인터럽트가 발생했음을 알리고 메시지를 출력
    vcBuffer[5] = '0' + iVectorNumber / 10;
    vcBuffer[6] = '0' + iVectorNumber % 10;
    // 발생한 횟수 출력
    vcBuffer[8] = '0' + g_iKeyboardInterruptCount;
    g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
    kPrintString(0, 0, vcBuffer);
    //===============================================================

    // 키보드 컨트롤러에서 데이터를 읽어서 아스키로 변환하여 큐에 삽입
    if(kIsOutputBufferFull() == TRUE){
        bTemp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue(bTemp);
    }

    // EOI 전송
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}