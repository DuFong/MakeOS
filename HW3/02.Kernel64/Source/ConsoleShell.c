#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"

// 커맨드 테이블 정의
SHELLCOMMANDENTRY gs_vstCommandTable[] = {
    {"help", "Show Help", kHelp},
    {"cls", "Clear Screen", kCls},
    {"totalram", "Show Total RAM Size", kShowTotalRAMSize},
    {"strtod", "String To Decimal/Hex Convert", kStringToDecimalHexTest},
    {"shutdown", "Shutdown And Reboot OS", kShutdown},
    {"raisefault", "Access And Write On Address 0x1ff000", kRaiseFault}
};

// 셸의 메인 루프
void kStartConsoleShell(){
    char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
    int iCommandBufferIndex = 0;
    BYTE bKey;
    int iCursorX, iCursorY;

    kPrintf(CONSOLESHELL_PROMPTMESSAGE);

    while(1){
        // 키가 수신될 때까지 대기
        bKey = kGetCh();
        
        if(bKey == KEY_BACKSPACE){
            if(iCommandBufferIndex > 0){
                kGetCursor(&iCursorX, &iCursorY);
                kPrintStringXY(iCursorX - 1, iCursorY, " ");
                kSetCursor(iCursorX - 1, iCursorY);
                iCommandBufferIndex--;
            }
        }
        else if(bKey == KEY_ENTER){
            kPrintf("\n");

            if(iCommandBufferIndex > 0){
                // 커맨드 버퍼에 있는 명령을 실행
                vcCommandBuffer[iCommandBufferIndex] = '\0';
                kExecuteCommand(vcCommandBuffer);
            }

            // 프롬프트 출력 및 커맨드 버퍼 초기화
            kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
            kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
            iCommandBufferIndex = 0;
        }
        else if((bKey == KEY_LSHIFT) || (bKey == KEY_RSHIFT) || (bKey == KEY_CAPSLOCK) || (bKey == KEY_NUMLOCK) || (bKey == KEY_SCROLLLOCK)){
            ;
        }
        else{
            if(bKey == KEY_TAB){
                bKey = ' ';
            }

            // 버퍼에 공간이 남아 있을때만 가능
            if(iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT){
                vcCommandBuffer[iCommandBufferIndex++] = bKey;
                kPrintf("%c", bKey);
            }
        }
    }
}

// 커맨드 버퍼에 있는 커맨드를 비교하여 해당 커맨드를 처리하는 함수를 수행
void kExecuteCommand(const char* pcCommandBuffer){
    int i, iSpaceIndex;
    int iCommandBufferLength, iCommandLength;
    int iCount;

    // 공백으로 구분된 커맨드를 추출
    iCommandBufferLength = kStrLen(pcCommandBuffer);
    for(iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++){
        if(pcCommandBuffer[iSpaceIndex] == ' '){
            break;
        }
    }

    // 커맨드 테이블을 검사해서 같은 이름의 커맨드가 있는지 확인
    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);
    for(i = 0; i < iCount; i++){
        iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        // 커맨드의 길이와 내용이 완전히 일치하는지 검사
        if((iCommandLength == iSpaceIndex) && (kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0)){
            gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1);
            break;
        }
    }

    // 리스트에서 찾을 수 없다면 에러 출력
    if(i >= iCount){
        kPrintf("'%s' is not found.\n", pcCommandBuffer);
    }
}

// 파라미터 자료구조를 초기화
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter){
    pstList->pcBuffer = pcParameter;
    pstList->iLength = kStrLen(pcParameter);
    pstList->iCurrentPosition = 0;
}

// 공백으로 구분된 파라미터의 내용과 길이를 반환
int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter){
    int i;
    int iLength;

    // 더 이상 파라미터가 없으면 나감
    if(pstList->iLength <= pstList->iCurrentPosition){
        return 0;
    }

    // 버퍼의 길이만큼 이동하면서 공백을 검색
    for(i = pstList->iCurrentPosition; i < pstList->iLength; i++){
        if(pstList->pcBuffer[i] == ' '){
            break;
        }
    }

    // 파라미터를 복사하고 길이를 반환
    kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
    iLength = i - pstList->iCurrentPosition;
    pcParameter[iLength] = '\0';

    // 파라미터의 위치 업데이트
    pstList->iCurrentPosition += iLength + 1;
    return iLength;
}

/* 커맨드를 처리하는 코드 */
// 셸 도움말 출력
void kHelp(const char* pcCommandBuffer){
    int i;
    int iCount;
    int iCursorX, iCursorY;
    int iLength, iMaxCommandLength = 0;

    kPrintf("=========================================\n");
    kPrintf("            MINT64 Shell Help            \n");
    kPrintf("=========================================\n");

    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

    // 가장 긴 커맨드의 길이를 계산
    for(i = 0; i < iCount; i++){
        iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        if(iLength > iMaxCommandLength){
            iMaxCommandLength = iLength;
        }
    }

    // 도움말 출력
    for(i = 0; i < iCount; i++){
        kPrintf("%s", gs_vstCommandTable[i].pcCommand);
        kGetCursor(&iCursorX, &iCursorY);
        kSetCursor(iMaxCommandLength, iCursorY);
        kPrintf(" - %s\n", gs_vstCommandTable[i].pcHelp);
    }
}

// 화면을 지움
void kCls(const char* pcParameterBuffer){
    // 맨 윗줄은 디버깅용으로 사용
    kClearScreen();
    kSetCursor(0, 1);
}

// 총 메모리 크기 출력
void kShowTotalRAMSize(const char* pcParameterBuffer){
    kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

// 문자열로 된 숫자를 숫자로 변환하여 화면에 출력
void kStringToDecimalHexTest(const char* pcParameterBuffer){
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    int iCount = 0;
    long lValue;

    kInitializeParameter(&stList, pcParameterBuffer);

    while(1){
        // 다음 파라미터를 구함, 파라미터의 길이가 0이면 파라미터가 없는 것이므로 종료
        iLength = kGetNextParameter(&stList, vcParameter);
        if(iLength == 0){
            break;
        }

        // 파라미터에 대한 정보를 출력하고 16진수인지 10진수인지 판단하여 변환한 후 결과 출력
        kPrintf("Param %d = '%s', Length = %d, ", iCount + 1, vcParameter, iLength);

        // 0x로 시작하면 16진수, 그 외에는 10진수로 판단
        if(kMemCmp(vcParameter, "0x", 2) == 0){
            lValue = kAToI(vcParameter + 2, 16);
            kPrintf("HEX Value = %q\n", lValue);
        }
        else{
            lValue = kAToI(vcParameter, 10);
            kPrintf("Decimal Value = %d\n", lValue);
        }
        
        iCount++;
    }
}

// PC 재시작
void kShutdown(const char* pcParameterBuffer){
    kPrintf("System Shutdown Start...\n");

    // 키보드 컨트롤러를 통해 PC 재시작
    kPrintf("Press Any Key To Reboot PC...");
    kGetCh();
    kReboot();
}

// 0x1ff000에 접근 및 쓰기
void kRaiseFault(const char* pcParameterBuffer){
    QWORD* address = 0x1FF000;

    // 접근
    DWORD test = *address;
    // 쓰기
    *address = 0x1234567;
}