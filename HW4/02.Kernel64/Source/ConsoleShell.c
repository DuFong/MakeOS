#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"
#include "AssemblyUtility.h"

// 커맨드 테이블 정의
SHELLCOMMANDENTRY gs_vstCommandTable[] = {
    {"help", "Show Help", kHelp},
    {"cls", "Clear Screen", kCls},
    {"totalram", "Show Total RAM Size", kShowTotalRAMSize},
    {"strtod", "String To Decimal/Hex Convert", kStringToDecimalHexTest},
    {"shutdown", "Shutdown And Reboot OS", kShutdown},
    {"raisefault", "Access And Write On Address 0x1ff000\nex)raisefault access\nex)raisefault write", kRaiseFault},
    {"settimer", "Set PIT Controller Counter0, ex)settimer 10(ms) 1(periodic)", kSetTimer},
    {"wait", "Wait ms Using PIT, ex)wait 100(ms)", kWaitUsingPIT},
    {"rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter},
    {"cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed},
    {"date", "Show Date And Time", kShowDateAndTime},
    {"createtask", "Create Task, ex)createtask 1(type) 10(count)", kCreateTestTask}

    // {"print1", "print1", kPrint1},
    // {"print2", "print2", kPrint2},
    // {"print3", "print3", kPrint3},
    // {"print11", "print11", kPrint11},
    // {"print22", "print22", kPrint22},
    // {"print33", "print33", kPrint33}
};

char historyCommand[10][100];
int idx = 0;        //enter쳤을 때, historyCommand[]에 삽입될 위치
int cnt = 0;        //historyCommand[]에 있는 명령어 수(10개를 넘어가면 10으로 고정)
int cidx = 0;       //up, down키 눌러서 가리키는 위치
int ccnt = 0;       //up키는 최대 10번 누를 수 있도록 check

// 셸의 메인 루프
void kStartConsoleShell(){
    char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
    int iCommandBufferIndex = 0;
    BYTE bKey = 0;
    BYTE bbKey = 0;     //이전에 입력한 키값
    int iCursorX, iCursorY;

    kPrintf(CONSOLESHELL_PROMPTMESSAGE);

    while(1){
        bbKey = bKey;
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

                kMemCpy(historyCommand[idx], vcCommandBuffer, iCommandBufferIndex + 1); //history배열을 잘못 덮어쓰는 문제 해결하기 위해 +1해줌
                if(++idx >= 10)
                    idx = 0;        //history배열 10칸이므로
                cidx = idx;
                ccnt = 0;
                if(cnt < 10)
                    cnt++;          //history배열에 들어있는 명령어 수

                kExecuteCommand(vcCommandBuffer);
            }

            // 프롬프트 출력 및 커맨드 버퍼 초기화
            kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
            kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
            iCommandBufferIndex = 0;
        }
        else if(bKey == KEY_TAB){
            //tab twice
            if ( bbKey == KEY_TAB)
            {
                if( iCommandBufferIndex > 0 )
                {
                    // if search results exist
                    if (kTabCommand( vcCommandBuffer ) == 1)
                    {
                        //print new prompt
                        kPrintf( "\n" );
                        kPrintf( "%s", CONSOLESHELL_PROMPTMESSAGE );
                        kPrintf( "%s", vcCommandBuffer );
                    }
                }
            }
            //tab once
            else if (bbKey != KEY_TAB)
            {
                if( iCommandBufferIndex > 0 )
                {
                    //initialize autocomplete string
                    char fillArr[10] = {'\0', };//'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
                    vcCommandBuffer[ iCommandBufferIndex ] = '\0';
                    kTabOnceCommand( vcCommandBuffer, fillArr);

                    //add autocomplete string into vcCommandBuffer
                    for (int i=0; i<kStrLen(fillArr); i++)
                    {
                        if (fillArr[i] != '\0')
                        {
                            vcCommandBuffer[iCommandBufferIndex++] = fillArr[i];
                        }
                    }
                }

            }
        }
        else if(bKey == KEY_UP){
            if(ccnt < cnt){         // 방향키 누른 횟수 check (최대10번)
                ccnt++;
                if(--cidx < 0)
                    cidx = 9;

                // 커서 얻어와서 그 줄 전체 지우기
                kGetCursor(&iCursorX, &iCursorY);
                for(int i=0; i<80; i++){
                    kPrintStringXY(i, iCursorY, " ");
                }

                // 프롬프트와 history배열의 cidx번째 명령어 출력
                kSetCursor(0, iCursorY);
                kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
                kPrintf( "%s", historyCommand[cidx]);

                // iCommandBufferIndex와 vcCommandBuffer 설정
                iCommandBufferIndex = kStrLen(historyCommand[cidx]);        //프롬프트 없어지는거 해결..
                kMemCpy(vcCommandBuffer, historyCommand[cidx], iCommandBufferIndex);

            }

        }
        else if(bKey == KEY_DOWN){
            if(ccnt > 0){       // 최대 up key누른 횟수만큼 down할 수 있음
                ccnt--;
                if(++cidx >= 10)
                    cidx = 0;

                // 커서 얻어와서 그 줄 전체 지우기
                kGetCursor(&iCursorX, &iCursorY);
                for(int i=0; i<80; i++){
                    kPrintStringXY(i, iCursorY, " ");
                }

                // 프롬프트 출력
                kSetCursor(0, iCursorY);
                kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);

                // history 배열 꽉차서 다른 값 나올 수 있음 => ccnt==0 이면 idx==cidx
                if(ccnt == 0){
                    // 커맨드 버퍼 초기화
                    kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
                    iCommandBufferIndex = 0;
                }
                else{
                    kPrintf( "%s", historyCommand[cidx]);

                    // iCommandBufferIndex와 vcCommandBuffer 설정
                    iCommandBufferIndex = kStrLen(historyCommand[cidx]);
                    kMemCpy(vcCommandBuffer, historyCommand[cidx], iCommandBufferIndex);
                }
            }
        }
        else if((bKey == KEY_LSHIFT) || (bKey == KEY_RSHIFT) || (bKey == KEY_CAPSLOCK) || (bKey == KEY_NUMLOCK) || (bKey == KEY_SCROLLLOCK)){
            ;
        }
        else{
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

int kTabCommand( const char* pcCommandBuffer )
{
    int i, iSpaceIndex;
    int iCommandBufferLength, iCommandLength;
    int iCount, tab2arr_index;

    // array to store candidate command's index
    int tab2[5] = {-1,};

    // check variable if there is matching command
    int check = 0;
    iCommandBufferLength = kStrLen( pcCommandBuffer );

    for( iSpaceIndex = 0 ; iSpaceIndex < iCommandBufferLength ; iSpaceIndex++ )
    {
        if( pcCommandBuffer[ iSpaceIndex ] == ' ' )
        {
            break;
        }
    }

    tab2arr_index = 0;
    iCount = sizeof( gs_vstCommandTable ) / sizeof( SHELLCOMMANDENTRY );

    for( i = 0 ; i < iCount ; i++ )
    {
        //if there is matching command, store its index in tab2
        if( kMemCmp( gs_vstCommandTable[ i ].pcCommand, pcCommandBuffer,
                       iSpaceIndex ) == 0 )
        {
            tab2[tab2arr_index] = i;
            tab2arr_index++;
            check = 1;
        }
    }
    if (check  == 1)
    {
        kPrintf("\n");
    }

    //print candidate commands
    for( int l = 0; l < tab2arr_index; l++)
    {
        int tmp = tab2[l];
        kPrintf("%s ", gs_vstCommandTable[tmp].pcCommand);
    }

    if (check  == 1)
    {
        kPrintf("\n");
    }

    return check;
}
void kTabOnceCommand( const char* pcCommandBuffer , char* fillArr)
{
    int i, iSpaceIndex;
    int iCommandBufferLength, iCommandLength;
    int iCount, tab2arr_index;
    int tab2[5] = {-1, };
    int minCommandLen = 100;

    iCommandBufferLength = kStrLen( pcCommandBuffer );
    for( iSpaceIndex = 0 ; iSpaceIndex < iCommandBufferLength ; iSpaceIndex++ )
    {
        if( pcCommandBuffer[ iSpaceIndex ] == ' ' )
        {
            break;
        }
    }

    tab2arr_index = 0;
    iCount = sizeof( gs_vstCommandTable ) / sizeof( SHELLCOMMANDENTRY );
    int check = 0;
    for( i = 0 ; i < iCount ; i++ )
    {
        if( kMemCmp( gs_vstCommandTable[ i ].pcCommand, pcCommandBuffer,
                       iSpaceIndex ) == 0 )
        {
            tab2[tab2arr_index] = i;
            tab2arr_index++;

            if ( kStrLen(gs_vstCommandTable[ i ].pcCommand) < minCommandLen)
            {
                minCommandLen = kStrLen(gs_vstCommandTable[ i ].pcCommand);
            }
            check = 1;
        }
        else
        {
            continue;
        }
    }
    if (check == 0)
    {
        return;
    }

    int tmp = tab2[0];
    int sync = iSpaceIndex;

    //find common string in candidate commands
    for (; sync < minCommandLen; sync++)
    {
        BOOL flag = 0;
        char stdChar = gs_vstCommandTable[tmp].pcCommand[sync];
        for (int k = 1; k < tab2arr_index; k++)
        {
            int tmp2 = tab2[k];
            if(stdChar != gs_vstCommandTable[tmp2].pcCommand[sync])
            {
                flag = 1;
                break;
            }
        }
        if (flag == 1)
        {
            break;
        }
    }

    int fillIdx = 0;

    //put autocomplete string in fillArr
    for (int i = iSpaceIndex; i<sync; i++)
    {
        char fill = gs_vstCommandTable[tmp].pcCommand[i];
        kPrintf("%c", fill);
        fillArr[fillIdx] = fill;
        fillIdx++;
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

// void kPrint1(const char* pcParameterBuffer){
//     kPrintf("pirnt1\n");
// }

// void kPrint2(const char* pcParameterBuffer){
//     kPrintf("pirnt2\n");
// }

// void kPrint3(const char* pcParameterBuffer){
//     kPrintf("pirnt3\n");
// }

// void kPrint11(const char* pcParameterBuffer){
//     kPrintf("pirnt11\n");
// }

// void kPrint22(const char* pcParameterBuffer){
//     kPrintf("pirnt22\n");
// }

// void kPrint33(const char* pcParameterBuffer){
//     kPrintf("pirnt33\n");
// }

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

    char* pcAccess = "access";
    char* pcWrite = "write";
    int i = 0;
    int accessCount = 0;
    int writeCount = 0;

    // access인지 write인지 검사
    while(pcParameterBuffer[i] != '\0'){
        if(pcParameterBuffer[i] == pcAccess[i]){
            accessCount++;
        }
        if(pcParameterBuffer[i] == pcWrite[i]){
            writeCount++;
        }
        i++;
    }

    // 접근
    if(accessCount == 6){
        DWORD test = *address;
    }
    // 쓰기
    else if(writeCount == 5){
        *address = 0x1234567;
    }
    else{
        kPrintf("%s is invalid parameter.\nraisefault access\nraisefault write\n", pcParameterBuffer);
    }
}

// PIT 컨트롤러의 카운터 0 설정
void kSetTimer(const char* pcParameterBuffer){
    char vcParameter[100];
    PARAMETERLIST stList;
    long lValue;
    BOOL bPeriodic;

    kInitializeParameter(&stList, pcParameterBuffer);

    // milisecond 추출
    if(kGetNextParameter(&stList, vcParameter) == 0){
        kPrintf("ex)settimer 10(ms) 1(periodic)\n");
        return;
    }
    lValue = kAToI(vcParameter, 10);

    // Periodic 추출
    if(kGetNextParameter(&stList, vcParameter) == 0){
        kPrintf("ex)settimer 10(ms) 1(periodic)\n");
        return;
    }
    bPeriodic = kAToI(vcParameter, 10);

    kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
    kPrintf("Time = %d ms, Periodic = %d Change Complete\n", lValue, bPeriodic);
}

// PIT 컨트롤러를 직접 사용하여 ms 동안 대기
void kWaitUsingPIT(const char* pcParameterBuffer){
    char vcParameter[100];
    int iLength;
    PARAMETERLIST stList;
    long lMillisecond;
    int i;

    kInitializeParameter(&stList, pcParameterBuffer);
    if(kGetNextParameter(&stList, vcParameter) == 0){
        kPrintf("ex)wait 100(ms)\n");
        return;
    }

    lMillisecond = kAToI(pcParameterBuffer, 10);
    kPrintf("%d ms Sleep Start...\n", lMillisecond);

    // 인터럽트를 비활성화하고 PIT 컨트롤러를 통해 직접 시간을 측정
    kDisableInterrupt();
    for(i = 0; i < lMillisecond / 30; i++){
        kWaitUsingDirectPIT(MSTOCOUNT(30));
    }
    kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
    kEnableInterrupt();
    kPrintf("%d ms Sleep Complete\n", lMillisecond);

    // 타이머 복원
    kInitializePIT(MSTOCOUNT(1), TRUE);
}

// 타임 스탬프 카운터를 읽음
void kReadTimeStampCounter(const char* pcParmeterBuffer){
    QWORD qwTSC;

    qwTSC = kReadTSC();
    kPrintf("Time Stamp Counter = %q\n", qwTSC);
}

// 프로세서의 속도를 측정
void kMeasureProcessorSpeed(const char* pcParameterBuffer){
    int i;
    QWORD qwLastTSC, qwTotalTSC = 0;

    kPrintf("Now Measuring.");

    // 10초 동안 변화한 타임 스탬프 카운터를 이용하여 프로세서의 속도를 간접적으로 측정
    kDisableInterrupt();
    for(i = 0; i < 200; i++){
        qwLastTSC = kReadTSC();
        kWaitUsingDirectPIT(MSTOCOUNT(50));
        qwTotalTSC += kReadTSC() - qwLastTSC;

        kPrintf(".");
    }

    // 타이머 복원
    kInitializePIT(MSTOCOUNT(1), TRUE);
    kEnableInterrupt();

    kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}

// RTC 컨트롤러에 저장된 일자 및 시간 정보를 표시
void kShowDateAndTime(const char* pcParameterBuffer){
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    // RTC 컨트롤러에서 시간 및 일자를 읽음
    kReadRTCTime(&bHour, &bMinute, &bSecond);
    kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

    kPrintf("Date: %d/%d/%d %s, ", wYear, bMonth, bDayOfMonth, kConvertDayOfWeekToString(bDayOfWeek));
    kPrintf("Time: %d:%d:%d\n", bHour, bMinute, bSecond);
}

static TCB gs_vstTask[2] = {0, };
static QWORD gs_vstStack[1024] = {0, };

// 태스크 전환을 테스트하는 태스크
void kTestTask(){
    int i = 0;

    while(1){
        // 메시지를 출력하고 키 입력을 대기
        kPrintf("[%d] This message is from kTestTask, Press any key to switch kConsoleShell~!!\n", i++);
        kGetCh();

        // 위에서 키가 입력되면 태스크를 전환
        kSwitchContext(&(gs_vstTask[1].stContext), &(gs_vstTask[0].stContext));
    }
}

// 화면 테두리를 돌면서 문자를 출력
void kTestTask1(){
    BYTE bData;
    int i = 0, iX = 0, iY = 0, iMargin;
    CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    pstRunningTask = kGetRunningTask();
    iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

    // 화면 네 귀퉁이를 돌면서 문자 출력
    while(1){
        switch(i){
            case 0:
                iX++;
                if(iX >= (CONSOLE_WIDTH - iMargin)){
                    i = 1;
                }
                break;

            case 1:
                iY++;
                if(iY >= (CONSOLE_HEIGHT - iMargin)){
                    i = 2;
                }
                break;
            
            case 2:
                iX--;
                if(iX < iMargin){
                    i = 3;
                }
                break;

            case 3:
                iY--;
                if(iY < iMargin){
                    i = 0;
                }
                break;
        }

        // 문자 및 색깔 지정
        pstScreen[iY * CONSOLE_WIDTH + iX].bCharactor = bData;
        pstScreen[iY * CONSOLE_WIDTH + iX].bAttribute = bData & 0x0F;
        bData++;

        // 다른 태스크로 전환
        kSchedule();
    }
}

// 자신의 ID를 참고하여 특정 위치에 회전하는 바람개비를 출력
void kTestTask2(){
    int i = 0, iOffset;
    CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;
    char vcData[4] = {'-', '\\', '|', '/'};

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    pstRunningTask = kGetRunningTask();
    iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    while(1){
        // 회전하는 바람개비 표시
        pstScreen[iOffset].bCharactor = vcData[i % 4];
        // 색깔 지정
        pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
        i++;

        kSchedule();
    }
}

// 태스크를 생성해서 멀티태스킹 수행
void kCreateTestTask(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcType[30];
    char vcCount[30];
    int i;

    // 파라미터 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcType);
    kGetNextParameter(&stList, vcCount);

    switch(kAToI(vcType, 10)){
        // Type1 task
        case 1:
            for(i = 0; i < kAToI(vcCount, 10); i++){
                if(kCreateTask(0, (QWORD) kTestTask1) == NULL){
                    break;
                }
            }

            kPrintf("Task1 %d Created\n", i);
            break;

        // Type2 task
        case 2:
        default:
            for(i = 0; i < kAToI(vcCount, 10); i++){
                if(kCreateTask(0, (QWORD) kTestTask2) == NULL){
                    break;
                }
            }

            kPrintf("Task2 %d Created\n", i);
            break;
    }
}