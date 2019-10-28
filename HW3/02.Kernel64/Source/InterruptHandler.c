#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"

// 페이지 폴트 출력
void kPrintPageFault(QWORD qwExceptionAddress){
    QWORD test = 15;
    kPrintf("=========================================\n");
    kPrintf("           Page Fault Occur~!!!!         \n");
    kPrintf("           Address: 0x%q\n", qwExceptionAddress);
    kPrintf("=========================================\n");
}

// protection fault 출력
void kPrintProtectionFault(QWORD qwExceptionAddress){
    kPrintf("=========================================\n");
    kPrintf("        Protection Fault Occur~!!!!      \n");
    kPrintf("           Address: 0x%q\n", qwExceptionAddress);
    kPrintf("=========================================\n");
}

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode){
    char vcBuffer[3] = {0, };

    // 인터럽트 벡터를 화면 오른쪽 위에 2자리 정수로 출력
    vcBuffer[0] = '0' + iVectorNumber / 10;
    vcBuffer[1] = '0' + iVectorNumber % 10;

    kPrintStringXY(0, 0, "=========================================");
    kPrintStringXY(0, 1, "            Exception Occur~!!!!         ");
    kPrintStringXY(0, 2, "                 Vector: ");
    kPrintStringXY(27, 2, vcBuffer);
    kPrintStringXY(0, 3, "=========================================");

    while(1);
}

void kPageFaultHandler(int iVectorNumber, QWORD qwErrorCode){
    char vcBuffer[7] = {0, };

    // 예외가 발생한 주소
    QWORD qwExceptionAddress = kGetExceptionAddress();

    // 에러 코드를 통해 protection 위반이 있었는지 확인
    if(qwErrorCode & 1){
        kPrintProtectionFault(qwExceptionAddress);
    }
    else{
        kPrintPageFault(qwExceptionAddress);
    }

    while(1);
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
    kPrintStringXY(70, 0, vcBuffer);
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
    kPrintStringXY(0, 0, vcBuffer);
    //===============================================================

    // 키보드 컨트롤러에서 데이터를 읽어서 아스키로 변환하여 큐에 삽입
    if(kIsOutputBufferFull() == TRUE){
        bTemp = kGetKeyboardScanCode();
        kConvertScanCodeAndPutQueue(bTemp);
    }

    // EOI 전송
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}