#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "Task.h"
#include "AssemblyUtility.h"
#include "Synchronization.h"
#include "ScreenSaver.h"
#include "DynamicMemory.h"
#include "HardDisk.h"

// 커맨드 테이블 정의
SHELLCOMMANDENTRY gs_vstCommandTable[] = {
    {"help", "Show Help", kHelp},
    {"cls", "Clear Screen", kCls},
    {"totalram", "Show Total RAM Size", kShowTotalRAMSize},
    {"strtod", "String To Decimal/Hex Convert", kStringToDecimalHexTest},
    {"shutdown", "Shutdown And Reboot OS", kShutdown},
    {"raisefault", "Access And Write On Address 0x1ff000\n\tex)raisefault access\n\tex)raisefault write", kRaiseFault},
    {"settimer", "Set PIT Controller Counter0, ex)settimer 10(ms) 1(periodic)", kSetTimer},
    {"wait", "Wait ms Using PIT, ex)wait 100(ms)", kWaitUsingPIT},
    {"rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter},
    {"cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed},
    {"date", "Show Date And Time", kShowDateAndTime},
    {"createtask", "Create Task, ex)createtask 1(type) 10(count) 1(priority)", kCreateTestTask},
    { "changepriority", "Change Task Priority, ex)changepriority 1(ID) 2(Priority)", kChangeTaskPriority },
    { "tasklist", "Show Task List", kShowTaskList },
    { "killtask", "End Task, ex)killtask 1(ID) or 0xffffffff(All Task)", kKillTask },
    { "cpuload", "Show Processor Load", kCPULoad },
    { "testmutex", "Test Mutex Function", kTestMutex },
    { "testthread", "Test Thread And Process Function, ex) testthread 3(Priority)", kTestThread },
    { "showmatrix", "Show Matrix Screen", kShowMatrix },
    {"prioritytask", "Create tasks with different priority", kPriorityTask},
    {"setscreentimer", "Set screen saver time, ex)setscreentimer 30(seconds)", kSetScreenTimer},
    { "dynamicmeminfo", "Show Dyanmic Memory Information", kShowDyanmicMemoryInformation },
    { "testseqalloc", "Test Sequential Allocation & Free", kTestSequentialAllocation },
    { "testranalloc", "Test Random Allocation & Free", kTestRandomAllocation },
    { "hddinfo", "Show HDD Information", kShowHDDInformation },
    { "readsector", "Read HDD Sector, ex)readsector 0(LBA) 10(count)", kReadSector },
    { "writesector", "Write HDD Sector, ex)writesector 0(LBA) 10(count)", kWriteSector },
};

char historyCommand[10][100];
int idx = 0;        //enter쳤을 때, historyCommand[]에 삽입될 위치
int cnt = 0;        //historyCommand[]에 있는 명령어 수(10개를 넘어가면 10으로 고정)
int cidx = 0;       //up, down키 눌러서 가리키는 위치
int ccnt = 0;       //up키는 최대 10번 누를 수 있도록 check

scrollDownPointer = 0;

// 셸의 메인 루프
void kStartConsoleShell(){
    char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
    int iCommandBufferIndex = 0;
    BYTE bKey = 0;
    BYTE bbKey = 0;     //이전에 입력한 키값
    int iCursorX, iCursorY;

    // 화면보호기 프로세스 생성
    kCreateScreenSaverTask();
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
        else if(bKey == KEY_F11){
            if (scrollUpPointer > 0){
                
                ////Store down row
                kMemCpy(SCROLLSAVEDOWN + scrollDownPointer * SCROLL_ROW , CONSOLE_VIDEOMEMORYADDRESS + (CONSOLE_HEIGHT - 1) * SCROLL_ROW,  SCROLL_ROW);

                scrollDownPointer++;

                //put (1) ~ (end-1) to (2) ~ (end) 
                krMemCpy(CONSOLE_VIDEOMEMORYADDRESS + SCROLL_ROW, CONSOLE_VIDEOMEMORYADDRESS, (CONSOLE_HEIGHT - 1) * SCROLL_ROW);
                
                //load stored (1)(up) row
                kMemCpy(CONSOLE_VIDEOMEMORYADDRESS, SCROLLSAVEUP + (scrollUpPointer - 1) * SCROLL_ROW , SCROLL_ROW);
                
                scrollUpPointer--;
            }
        }
        else if(bKey == KEY_F12){
            if(scrollDownPointer > 0){
                
                scrollUpPointer++;
                
                //put (2) ~ (end) to (1) ~ (end-1)
                kMemCpy(CONSOLE_VIDEOMEMORYADDRESS , CONSOLE_VIDEOMEMORYADDRESS + SCROLL_ROW, (CONSOLE_HEIGHT - 1) * SCROLL_ROW);            

                kMemCpy(CONSOLE_VIDEOMEMORYADDRESS + (CONSOLE_HEIGHT - 1) * SCROLL_ROW, SCROLLSAVEDOWN + (scrollDownPointer - 1) * SCROLL_ROW , SCROLL_ROW);
                
                scrollDownPointer--;

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

// 셸 도움말 출력
static void kHelp(const char* pcCommandBuffer){
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

        // 목록이 많을 경우 나눠서 보여줌
        if( ( i != 0 ) && ( ( i % 20 ) == 0 ) )
        {
            kPrintf( "Press any key to continue... ('q' is exit) : " );
            if( kGetCh() == 'q' )
            {
                kPrintf( "\n" );
                break;
            }        
            kPrintf( "\n" );
        }
    }
}

// 화면을 지움
static void kCls(const char* pcParameterBuffer){
    // 맨 윗줄은 디버깅용으로 사용
    kClearScreen();
    kSetCursor(0, 1);
}

// 총 메모리 크기 출력
static void kShowTotalRAMSize(const char* pcParameterBuffer){
    kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

// 문자열로 된 숫자를 숫자로 변환하여 화면에 출력
static void kStringToDecimalHexTest(const char* pcParameterBuffer){
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
static void kShutdown(const char* pcParameterBuffer){
    kPrintf("System Shutdown Start...\n");

    // 키보드 컨트롤러를 통해 PC 재시작
    kPrintf("Press Any Key To Reboot PC...");
    kGetCh();
    kReboot();
}

// 0x1ff000에 접근 및 쓰기
static void kRaiseFault(const char* pcParameterBuffer){
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
static void kSetTimer(const char* pcParameterBuffer){
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
static void kWaitUsingPIT(const char* pcParameterBuffer){
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
static void kReadTimeStampCounter(const char* pcParmeterBuffer){
    QWORD qwTSC;

    qwTSC = kReadTSC();
    kPrintf("Time Stamp Counter = %q\n", qwTSC);
}

// 프로세서의 속도를 측정
static void kMeasureProcessorSpeed(const char* pcParameterBuffer){
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
static void kShowDateAndTime(const char* pcParameterBuffer){
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    // RTC 컨트롤러에서 시간 및 일자를 읽음
    kReadRTCTime(&bHour, &bMinute, &bSecond);
    kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

    kPrintf("Date: %d/%d/%d %s, ", wYear, bMonth, bDayOfMonth, kConvertDayOfWeekToString(bDayOfWeek));
    kPrintf("Time: %d:%d:%d\n", bHour, bMinute, bSecond);
}

// 화면 테두리를 돌면서 문자를 출력
static void kTestTask1(){
    BYTE bData;
    int i = 0, iX = 0, iY = 0, iMargin, j;
    CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    pstRunningTask = kGetRunningTask();
    iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

    // 화면 네 귀퉁이를 돌면서 문자 출력
    for( j = 0 ; j < 20000 ; j++ )
    {
        // 화면보호기가 켜져있을 때는 태스크를 실행시키지 않음
        if(g_stScreenSaver.bScreenSaverOn){
            kSchedule();
        }

        switch( i )
        {
        case 0:
            iX++;
            if( iX >= ( CONSOLE_WIDTH - iMargin ) ){
                i = 1;
            }
            break;
            
        case 1:
            iY++;
            if( iY >= ( CONSOLE_HEIGHT - iMargin ) ){
                i = 2;
            }
            break;
            
        case 2:
            iX--;
            if( iX < iMargin ){
                i = 3;
            }
            break;
            
        case 3:
            iY--;
            if( iY < iMargin ){
                i = 0;
            }
            break;
        }
        
        // 문자 및 색깔 지정
        pstScreen[ iY * CONSOLE_WIDTH + iX ].bCharactor = bData;
        pstScreen[ iY * CONSOLE_WIDTH + iX ].bAttribute = bData & 0x0F;
        bData++;
        
        // 다른 태스크로 전환
        //kSchedule();
    }

    kExitTask();
}

// 자신의 ID를 참고하여 특정 위치에 회전하는 바람개비를 출력
static void kTestTask2(){
    int i = 0, iOffset;
    CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
    TCB* pstRunningTask;
    char vcData[4] = {'-', '\\', '|', '/'};
    QWORD qwLastSpendTickInIdleTask, qwLastMeasureTickCount, qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    pstRunningTask = kGetRunningTask();
    iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
    iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

    // 프로세서 사용량 계산을 위해 기준 정보를 저장
    qwLastSpendTickInIdleTask = pstRunningTask->qwProcessorTime;
    qwLastMeasureTickCount = kGetTickCount();

    while(1){
        // 화면보호기가 켜져있을 때는 태스크를 실행시키지 않음
        if(g_stScreenSaver.bScreenSaverOn){
            kSchedule();
        }

        // 현재 상태를 저장
        qwCurrentMeasureTickCount = kGetTickCount();
        qwCurrentSpendTickInIdleTask = pstRunningTask->qwProcessorTime;

        // 프로세서 점유율 계산
        if( qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0 )
        {
            pstRunningTask->qwProcessorShare = 0;
        }
        else
        {
            pstRunningTask->qwProcessorShare = (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) * 100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount);
        }

        
        
        // 회전하는 바람개비 표시
        pstScreen[iOffset].bCharactor = vcData[i % 4];
        // 색깔 지정
        pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
        i++;

        // 다른 태스크로 전환
        //kSchedule()
    }
}

// 태스크를 생성해서 멀티태스킹 수행
static void kCreateTestTask(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcType[30];
    char vcCount[30];
    char vcPriority[30];
    BYTE bPriority;
    int i;

    // 파라미터 추출
    kInitializeParameter(&stList, pcParameterBuffer);
    kGetNextParameter(&stList, vcType);
    kGetNextParameter(&stList, vcCount);
    kGetNextParameter(&stList, vcPriority);
    bPriority = vcPriority[0] - '0';
    // 우선순위를 입력하지 않으면 종료
    if(bPriority < 0 && bPriority > 4){
        kPrintf("Invalid priority. ex)createtask 1(type) 10(count) 1(priority)\n");
        return;
    }

    switch(kAToI(vcType, 10)){
        // Type1 task
        case 1:
            for( i = 0 ; i < kAToI( vcCount, 10 ) ; i++ ){    
                if( kCreateTask( bPriority | TASK_FLAGS_THREAD, 0, 0, (QWORD) kTestTask1 ) == NULL ){
                    break;
                }
            }
            kPrintf("Task1 %d Created\n", i);
            break;

        // Type2 task
        case 2:
        default:
            for(i = 0; i < kAToI(vcCount, 10); i++){
                if( kCreateTask(bPriority | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask2 ) == NULL){
                    break;
                }
            }
            kPrintf("Task2 %d Created\n", i);
            break;
    }
}

/**
 *  태스크의 우선 순위를 변경
 */
static void kChangeTaskPriority( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcID[ 30 ];
    char vcPriority[ 30 ];
    QWORD qwID;
    BYTE bPriority;
    
    // 파라미터를 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcID );
    kGetNextParameter( &stList, vcPriority );
    
    // 태스크의 우선 순위를 변경
    if( kMemCmp( vcID, "0x", 2 ) == 0 )
    {
        qwID = kAToI( vcID + 2, 16 );
    }
    else
    {
        qwID = kAToI( vcID, 10 );
    }
    
    bPriority = kAToI( vcPriority, 10 );
    
    kPrintf( "Change Task Priority ID [0x%q] Priority[%d] ", qwID, bPriority );
    if( kChangePriority( qwID, bPriority ) == TRUE )
    {
        kPrintf( "Success\n" );
    }
    else
    {
        kPrintf( "Fail\n" );
    }
}

/**
 *  현재 생성된 모든 태스크의 정보를 출력
 */
static void kShowTaskList( const char* pcParameterBuffer )
{
    int i;
    TCB* pstTCB;
    int iCount = 0;
    QWORD qwTickCount = kGetTickCount();
    
    kPrintf( "=========== Task Total Count [%d] ===========\n", kGetTaskCount() ); 
    kPrintf("Elapsed Time : %d.%ds\n", qwTickCount / 1000, qwTickCount / 100);
    // 생성한 태스크끼리 fairness를 비교하기 위해 셸과 유휴태스크는 출력하지 않음
    for( i = 2 ; i < TASK_MAXCOUNT ; i++ )
    {
        // TCB를 구해서 TCB가 사용 중이면 ID를 출력
        pstTCB = kGetTCBInTCBPool( i );
        if( ( pstTCB->stLink.qwID >> 32 ) != 0 )
        {
            // 태스크가 10개 출력될 때마다, 계속 태스크 정보를 표시할지 여부를 확인
            if( ( iCount != 0 ) && ( ( iCount % 18 ) == 0 ) )
            {
                kPrintf( "Press any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    kPrintf( "\n" );
                    break;
                }
                kPrintf( "\n" );
            }
            
            kPrintf("[%d] ID[0x%Q], Priority[%d], Processor Share[%d%%], Processor Time[%d]\n", 
                1 + iCount++, pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags), pstTCB->qwProcessorShare, pstTCB->qwProcessorTime);

            // kPrintf( "[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n", 1 + iCount++,
            //          pstTCB->stLink.qwID, GETPRIORITY( pstTCB->qwFlags ), 
            //          pstTCB->qwFlags, kGetListCount( &( pstTCB->stChildThreadList ) ) );
            // kPrintf( "    Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n",
            //         pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize );
            // kPrintf("Processor Time[%d]\n", pstTCB->qwProcessorTime);
        }
    }
}

/**
 *  태스크를 종료
 */
static void kKillTask( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcID[ 30 ];
    QWORD qwID;
    TCB* pstTCB;
    int i;
    
    // 파라미터를 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcID );
    
    // 태스크를 종료
    if( kMemCmp( vcID, "0x", 2 ) == 0 )
    {
        qwID = kAToI( vcID + 2, 16 );
    }
    else
    {
        qwID = kAToI( vcID, 10 );
    }
    
    // 특정 ID만 종료하는 경우
    if( qwID != 0xFFFFFFFF )
    {
        pstTCB = kGetTCBInTCBPool( GETTCBOFFSET( qwID ) );
        qwID = pstTCB->stLink.qwID;

        // 시스템 테스트는 제외
        if( ( ( qwID >> 32 ) != 0 ) && ( ( pstTCB->qwFlags & TASK_FLAGS_SYSTEM ) == 0x00 ) )
        {
            kPrintf( "Kill Task ID [0x%q] ", qwID );
            if( kEndTask( qwID ) == TRUE )
            {
                kPrintf( "Success\n" );
            }
            else
            {
                kPrintf( "Fail\n" );
            }
        }
        else
        {
            kPrintf( "Task does not exist or task is system task\n" );
        }
    }
    // 콘솔 셸과 유휴 태스크를 제외하고 모든 태스크 종료
    else
    {
        for( i = 0 ; i < TASK_MAXCOUNT ; i++ )
        {
            pstTCB = kGetTCBInTCBPool( i );
            qwID = pstTCB->stLink.qwID;

            // 시스템 테스트는 삭제 목록에서 제외
            if( ( ( qwID >> 32 ) != 0 ) && ( ( pstTCB->qwFlags & TASK_FLAGS_SYSTEM ) == 0x00 ) )
            {
                kPrintf( "Kill Task ID [0x%q] ", qwID );
                if( kEndTask( qwID ) == TRUE )
                {
                    kPrintf( "Success\n" );
                }
                else
                {
                    kPrintf( "Fail\n" );
                }
            }
        }
    }
}

/**
 *  프로세서의 사용률을 표시
 */
static void kCPULoad( const char* pcParameterBuffer )
{
    kPrintf( "Processor Load : %d%%\n", kGetProcessorLoad() );
}

// 뮤텍스 테스트용 뮤텍스와 변수
static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

/**
 *  뮤텍스를 테스트하는 태스크
 */
static void kPrintNumberTask( void )
{
    int i;
    int j;
    QWORD qwTickCount;

    // 50ms 정도 대기하여 콘솔 셸이 출력하는 메시지와 겹치지 않도록 함
    qwTickCount = kGetTickCount();
    while( ( kGetTickCount() - qwTickCount ) < 50 )
    {
        kSchedule();
    }    
    
    // 루프를 돌면서 숫자를 출력
    for( i = 0 ; i < 5 ; i++ )
    {
        kLock( &( gs_stMutex ) );
        kPrintf( "Task ID [0x%Q] Value[%d]\n", kGetRunningTask()->stLink.qwID,
                gs_qwAdder );
        
        gs_qwAdder += 1;
        kUnlock( & ( gs_stMutex ) );
    
        // 프로세서 소모를 늘리려고 추가한 코드
        for( j = 0 ; j < 30000 ; j++ ) ;
    }
    
    // 모든 태스크가 종료할 때까지 1초(100ms) 정도 대기
    qwTickCount = kGetTickCount();
    while( ( kGetTickCount() - qwTickCount ) < 1000 )
    {
        kSchedule();
    }    
    
    // 태스크 종료
    kExitTask();
}

/**
 *  뮤텍스를 테스트하는 태스크 생성
 */
static void kTestMutex( const char* pcParameterBuffer )
{
    int i;
    
    gs_qwAdder = 1;
    
    // 뮤텍스 초기화
    kInitializeMutex( &gs_stMutex );
    
    for( i = 0 ; i < 3 ; i++ )
    {
        // 뮤텍스를 테스트하는 태스크를 3개 생성
        kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kPrintNumberTask );
    }    
    kPrintf( "Wait Util %d Task End...\n", i );
    kGetCh();
}

/**
 *  태스크 2를 자신의 스레드로 생성하는 태스크
 */
static void kCreateThreadTask( void )
{
    int i;
    
    for( i = 0 ; i < 3 ; i++ )
    {
        kCreateTask( TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kTestTask2 );
    }
    
    while( 1 )
    {
        kSleep( 1 );
    }
}

/**
 *  스레드를 테스트하는 태스크 생성
 */
static void kTestThread( const char* pcParameterBuffer )
{
    TCB* pstProcess;
    BYTE bPriority;

    if(pcParameterBuffer == NULL){
        kPrintf("Invalid priority. ex)testthread 3(Priority)");
        return;
    }
    // 우선순위를 입력받음
    bPriority = pcParameterBuffer[0] - '0';
    
    pstProcess = kCreateTask( bPriority | TASK_FLAGS_PROCESS, ( void * )0xEEEEEEEE, 0x1000, ( QWORD ) kCreateThreadTask );
    if( pstProcess != NULL )
    {
        kPrintf( "Process [0x%Q] Create Success\n", pstProcess->stLink.qwID ); 
    }
    else
    {
        kPrintf( "Process Create Fail\n" );
    }
}

// 난수를 발생시키기 위한 변수
static volatile QWORD gs_qwRandomValue = 0;

/**
 *  임의의 난수를 반환
 */
QWORD kRandom( void )
{
    gs_qwRandomValue = ( gs_qwRandomValue * 412153 + 5571031 ) >> 16;
    return gs_qwRandomValue;
}

/**
 *  철자를 흘러내리게 하는 스레드
 */
static void kDropCharactorThread( void )
{
    int iX, iY;
    int i;
    char vcText[ 2 ] = { 0, };

    iX = kRandom() % CONSOLE_WIDTH;
    
    while( 1 )
    {
        // 잠시 대기함
        kSleep( kRandom() % 20 );
        
        if( ( kRandom() % 20 ) < 16 )
        {
            vcText[ 0 ] = ' ';
            for( i = 0 ; i < CONSOLE_HEIGHT ; i++ )
            {
                kPrintStringXY( iX, i , vcText );
                kSleep( 50 );
            }
        }        
        else
        {
            for( i = 0 ; i < CONSOLE_HEIGHT ; i++ )
            {
                vcText[ 0 ] = i + kRandom();
                kPrintStringXY( iX, i, vcText );
                kSleep( 50 );
            }
        }
    }
}

/**
 *  스레드를 생성하여 매트릭스 화면처럼 보여주는 프로세스
 */
static void kMatrixProcess( void )
{
    int i;
    
    for( i = 0 ; i < 300 ; i++ )
    {
        if( kCreateTask( TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, 
                         ( QWORD ) kDropCharactorThread ) == NULL )
        {
            break;
        }
        kPrintf("a");
        
        kSleep( kRandom() % 5 + 5 );
    }
    
    //kPrintf( "%d Thread is created\n", i );

    // 키가 입력되면 프로세스 종료
    kGetCh();
}

/**
 *  매트릭스 화면을 보여줌
 */
static void kShowMatrix( const char* pcParameterBuffer )
{
    TCB* pstProcess;
    
    pstProcess = kCreateTask( TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, ( void* ) 0xE00000, 0xE00000,
                              ( QWORD ) kMatrixProcess );
    if( pstProcess != NULL )
    {
        kPrintf( "Matrix Process [0x%Q] Create Success\n" );

        // 태스크가 종료 될 때까지 대기
        while( ( pstProcess->stLink.qwID >> 32 ) != 0 )
        {
            kSleep( 100 );
        }
    }
    else
    {
       // kPrintf( "Matrix Process Create Fail\n" );
    }
}

/**
 * 우선순위가 다른 태스크를 만듦(현재 환경에서는 5개의 우선순위)
 */
static void kPriorityTask(const char* pcParameterBuffer){
    TCB* pstProcess;
    int i, j;

    for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++){
        // 우선순위별로 태스크를 3개씩 생성
        for(j = 0; j < 3; j++){
            pstProcess = kCreateTask( i | TASK_FLAGS_THREAD, 0, 0 , ( QWORD ) kTestTask2 );

            if( pstProcess != NULL )
            {
                kPrintf( "Process [0x%Q] Create Success\n", pstProcess->stLink.qwID ); 
            }
            else
            {
                kPrintf( "Process Create Fail\n" );
            }
        } 
    }  
}

/**
 * 화면보호기 시간 설정
 */
static void kSetScreenTimer(const char* pcParameterBuffer){
    PARAMETERLIST stList;
    char vcID[ 30 ];

    if(pcParameterBuffer == NULL){
        kPrintf("Invalid time. ex) setscreentimer 30(seconds)\n");
        return;
    }
    
    // 파라미터를 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    kGetNextParameter( &stList, vcID );

    // 초 단위를 ms 단위로 변환
    g_stScreenSaver.qwRestrictionScreenOn = (QWORD) kAToI(vcID, 10) * 1000;
}

// 화면보호기 프로세스, 화면에 랜덤 색으로 랜덤 글자를 출력함
static void kScreenSaverProcess(){
    int iX, iY, iTotal, i;
    char vcText[ 2 ] = { 0, };
    CHARACTER* pstScreen = (CHARACTER*) CONSOLE_VIDEOMEMORYADDRESS;
    
    while(1){
        if(g_stScreenSaver.bScreenSaverOn){
            for(i = 0; i < 10; i++){
                if(kGetTickCount() % 10 == 0){
                    iX = kRandom() % CONSOLE_WIDTH;
                    iY = kRandom() % CONSOLE_HEIGHT;

                    if( ( kRandom() % 20 ) < 16 ){
                        vcText[0] = ' ';
                    }
                    else{
                        vcText[0] = i + kRandom();
                    }

                    kPrintStringXY(iX, iY, vcText);
                    pstScreen[ iY * CONSOLE_WIDTH + iX ].bAttribute = i & 0x0F;
                }
            }      
        }       
    }
}

static void kCreateScreenSaverTask(){
    TCB* pstProcess;
    
    pstProcess = kCreateTask( TASK_FLAGS_PROCESS | TASK_FLAGS_HIGHEST, ( void* ) 0xE00000, 0xE00000,
                              ( QWORD ) kScreenSaverProcess );

    if(pstProcess == NULL){
        kPrintf("Cannot execute the screen saver\n");
    }
    else{
        g_stScreenSaver.pstProcess = pstProcess;
    }
}



void kCallCls(){
    kCls(NULL);
}

void kCallTaskList(){
    kShowTaskList(NULL);
}

// 화면보호기 실행
void kScreenSaverOn(){
    // 화면보호기 실행 전 커서 위치 저장
    kGetCursor(&g_stScreenSaver.stCursor.iX, &g_stScreenSaver.stCursor.iY);

    // 화면보호기 실행 전 화면 저장
    kMemCpy(g_stScreenSaver.vcScreen, CONSOLE_VIDEOMEMORYADDRESS, CONSOLE_WIDTH * CONSOLE_HEIGHT * 2);

    g_stScreenSaver.bScreenSaverOn = TRUE;
}

// 화면보호기 종료
void kScreenSaverOff(){
    g_stScreenSaver.bScreenSaverOn = FALSE;

    // // 화면보호기 실행 전 커서 위치 복구
    kSetCursor(g_stScreenSaver.stCursor.iX, g_stScreenSaver.stCursor.iY);

    // 화면보호기 실행 전 화면 복구
    kMemCpy(CONSOLE_VIDEOMEMORYADDRESS, g_stScreenSaver.vcScreen, CONSOLE_WIDTH * CONSOLE_HEIGHT * 2);
}

/**
 *  동적 메모리 정보를 표시
 */
static void kShowDyanmicMemoryInformation( const char* pcParameterBuffer )
{
    QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;
    
    kGetDynamicMemoryInformation( &qwStartAddress, &qwTotalSize, &qwMetaSize, 
            &qwUsedSize );

    kPrintf( "============ Dynamic Memory Information ============\n" );
    kPrintf( "Start Address: [0x%Q]\n", qwStartAddress );
    kPrintf( "Total Size:    [0x%Q]byte, [%d]MB\n", qwTotalSize, 
            qwTotalSize / 1024 / 1024 );
    kPrintf( "Meta Size:     [0x%Q]byte, [%d]KB\n", qwMetaSize, 
            qwMetaSize / 1024 );
    kPrintf( "Used Size:     [0x%Q]byte, [%d]KB\n", qwUsedSize, qwUsedSize / 1024 );
}

/**
 *  모든 블록 리스트의 블록을 순차적으로 할당하고 해제하는 테스트
 */
static void kTestSequentialAllocation( const char* pcParameterBuffer )
{
    DYNAMICMEMORY* pstMemory;
    long i, j, k;
    QWORD* pqwBuffer;
    
    kPrintf( "============ Dynamic Memory Test ============\n" );
    pstMemory = kGetDynamicMemoryManager();
    
    for( i = 0 ; i < pstMemory->iMaxLevelCount ; i++ )
    {
        kPrintf( "Block List [%d] Test Start\n", i );
        kPrintf( "Allocation And Compare: ");
        
        // 모든 블록을 할당 받아서 값을 채운 후 검사
        for( j = 0 ; j < ( pstMemory->iBlockCountOfSmallestBlock >> i ) ; j++ )
        {
            pqwBuffer = kAllocateMemory( DYNAMICMEMORY_MIN_SIZE << i );
            if( pqwBuffer == NULL )
            {
                kPrintf( "\nAllocation Fail\n" );
                return ;
            }

            // 값을 채운 후 다시 검사
            for( k = 0 ; k < ( DYNAMICMEMORY_MIN_SIZE << i ) / 8 ; k++ )
            {
                pqwBuffer[ k ] = k;
            }
            
            for( k = 0 ; k < ( DYNAMICMEMORY_MIN_SIZE << i ) / 8 ; k++ )
            {
                if( pqwBuffer[ k ] != k )
                {
                    kPrintf( "\nCompare Fail\n" );
                    return ;
                }
            }
            // 진행 과정을 . 으로 표시
            kPrintf( "." );
        }
        
        kPrintf( "\nFree: ");
        // 할당 받은 블록을 모두 반환
        for( j = 0 ; j < ( pstMemory->iBlockCountOfSmallestBlock >> i ) ; j++ )
        {
            if( kFreeMemory( ( void * ) ( pstMemory->qwStartAddress + 
                         ( DYNAMICMEMORY_MIN_SIZE << i ) * j ) ) == FALSE )
            {
                kPrintf( "\nFree Fail\n" );
                return ;
            }
            // 진행 과정을 . 으로 표시
            kPrintf( "." );
        }
        kPrintf( "\n" );
    }
    kPrintf( "Test Complete~!!!\n" );
}

/**
 *  임의로 메모리를 할당하고 해제하는 것을 반복하는 태스크
 */
static void kRandomAllocationTask( void )
{
    TCB* pstTask;
    QWORD qwMemorySize;
    char vcBuffer[ 200 ];
    BYTE* pbAllocationBuffer;
    int i, j;
    int iY;
    
    pstTask = kGetRunningTask();
    iY = ( pstTask->stLink.qwID ) % 15 + 9;

    for( j = 0 ; j < 10 ; j++ )
    {
        // 1KB ~ 32M까지 할당하도록 함
        do
        {
            qwMemorySize = ( ( kRandom() % ( 32 * 1024 ) ) + 1 ) * 1024;
            pbAllocationBuffer = kAllocateMemory( qwMemorySize );

            // 만일 버퍼를 할당 받지 못하면 다른 태스크가 메모리를 사용하고 
            // 있을 수 있으므로 잠시 대기한 후 다시 시도
            if( pbAllocationBuffer == 0 )
            {
                kSleep( 1 );
            }
        } while( pbAllocationBuffer == 0 );
            
        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Allocation Success", 
                  pbAllocationBuffer, qwMemorySize );
        // 자신의 ID를 Y 좌표로 하여 데이터를 출력
        kPrintStringXY( 20, iY, vcBuffer );
        kSleep( 200 );
        
        // 버퍼를 반으로 나눠서 랜덤한 데이터를 똑같이 채움 
        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Write...     ", 
                  pbAllocationBuffer, qwMemorySize );
        kPrintStringXY( 20, iY, vcBuffer );
        for( i = 0 ; i < qwMemorySize / 2 ; i++ )
        {
            pbAllocationBuffer[ i ] = kRandom() & 0xFF;
            pbAllocationBuffer[ i + ( qwMemorySize / 2 ) ] = pbAllocationBuffer[ i ];
        }
        kSleep( 200 );
        
        // 채운 데이터가 정상적인지 다시 확인
        kSPrintf( vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Verify...   ", 
                  pbAllocationBuffer, qwMemorySize );
        kPrintStringXY( 20, iY, vcBuffer );
        for( i = 0 ; i < qwMemorySize / 2 ; i++ )
        {
            if( pbAllocationBuffer[ i ] != pbAllocationBuffer[ i + ( qwMemorySize / 2 ) ] )
            {
                kPrintf( "Task ID[0x%Q] Verify Fail\n", pstTask->stLink.qwID );
                kExitTask();
            }
        }
        kFreeMemory( pbAllocationBuffer );
        kSleep( 200 );
    }
    
    kExitTask();
}

/**
 *  태스크를 여러 개 생성하여 임의의 메모리를 할당하고 해제하는 것을 반복하는 테스트
 */
static void kTestRandomAllocation( const char* pcParameterBuffer )
{
    int i;
    
    for( i = 0 ; i < 1000 ; i++ )
    {
        kCreateTask( TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, ( QWORD ) kRandomAllocationTask );
    }
}

/**
 *  하드 디스크의 정보를 표시
 */
static void kShowHDDInformation( const char* pcParameterBuffer )
{
    HDDINFORMATION stHDD;
    char vcBuffer[ 100 ];
    
    // 하드 디스크의 정보를 읽음
    if( kReadHDDInformation( TRUE, TRUE, &stHDD ) == FALSE )
    {
        kPrintf( "HDD Information Read Fail\n" );
        return ;
    }        
    
    kPrintf( "============ Primary Master HDD Information ============\n" );
    
    // 모델 번호 출력
    kMemCpy( vcBuffer, stHDD.vwModelNumber, sizeof( stHDD.vwModelNumber ) );
    vcBuffer[ sizeof( stHDD.vwModelNumber ) - 1 ] = '\0';
    kPrintf( "Model Number:\t %s\n", vcBuffer );
    
    // 시리얼 번호 출력
    kMemCpy( vcBuffer, stHDD.vwSerialNumber, sizeof( stHDD.vwSerialNumber ) );
    vcBuffer[ sizeof( stHDD.vwSerialNumber ) - 1 ] = '\0';
    kPrintf( "Serial Number:\t %s\n", vcBuffer );

    // 헤드, 실린더, 실린더 당 섹터 수를 출력
    kPrintf( "Head Count:\t %d\n", stHDD.wNumberOfHead );
    kPrintf( "Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder );
    kPrintf( "Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder );
    
    // 총 섹터 수 출력
    kPrintf( "Total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors, 
            stHDD.dwTotalSectors / 2 / 1024 );
}

/**
 *  하드 디스크에 파라미터로 넘어온 LBA 어드레스에서 섹터 수 만큼 읽음
 */
static void kReadSector( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcLBA[ 50 ], vcSectorCount[ 50 ];
    DWORD dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    BYTE bData;
    BOOL bExit = FALSE;
    
    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    if( ( kGetNextParameter( &stList, vcLBA ) == 0 ) ||
        ( kGetNextParameter( &stList, vcSectorCount ) == 0 ) )
    {
        kPrintf( "ex) readsector 0(LBA) 10(count)\n" );
        return ;
    }
    dwLBA = kAToI( vcLBA, 10 );
    iSectorCount = kAToI( vcSectorCount, 10 );
    
    // 섹터 수만큼 메모리를 할당 받아 읽기 수행
    pcBuffer = kAllocateMemory( iSectorCount * 512 );
    if( kReadHDDSector( TRUE, TRUE, dwLBA, iSectorCount, pcBuffer ) == iSectorCount )
    {
        kPrintf( "LBA [%d], [%d] Sector Read Success~!!", dwLBA, iSectorCount );
        // 데이터 버퍼의 내용을 출력
        for( j = 0 ; j < iSectorCount ; j++ )
        {
            for( i = 0 ; i < 512 ; i++ )
            {
                if( !( ( j == 0 ) && ( i == 0 ) ) && ( ( i % 256 ) == 0 ) )
                {
                    kPrintf( "\nPress any key to continue... ('q' is exit) : " );
                    if( kGetCh() == 'q' )
                    {
                        bExit = TRUE;
                        break;
                    }
                }                

                if( ( i % 16 ) == 0 )
                {
                    kPrintf( "\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i ); 
                }

                // 모두 두 자리로 표시하려고 16보다 작은 경우 0을 추가해줌
                bData = pcBuffer[ j * 512 + i ] & 0xFF;
                if( bData < 16 )
                {
                    kPrintf( "0" );
                }
                kPrintf( "%X ", bData );
            }
            
            if( bExit == TRUE )
            {
                break;
            }
        }
        kPrintf( "\n" );
    }
    else
    {
        kPrintf( "Read Fail\n" );
    }
    
    kFreeMemory( pcBuffer );
}

/**
 *  하드 디스크에 파라미터로 넘어온 LBA 어드레스에서 섹터 수 만큼 씀
 */
static void kWriteSector( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcLBA[ 50 ], vcSectorCount[ 50 ];
    DWORD dwLBA;
    int iSectorCount;
    char* pcBuffer;
    int i, j;
    BOOL bExit = FALSE;
    BYTE bData;
    static DWORD s_dwWriteCount = 0;

    // 파라미터 리스트를 초기화하여 LBA 어드레스와 섹터 수 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    if( ( kGetNextParameter( &stList, vcLBA ) == 0 ) ||
        ( kGetNextParameter( &stList, vcSectorCount ) == 0 ) )
    {
        kPrintf( "ex) writesector 0(LBA) 10(count)\n" );
        return ;
    }
    dwLBA = kAToI( vcLBA, 10 );
    iSectorCount = kAToI( vcSectorCount, 10 );

    s_dwWriteCount++;
    
    // 버퍼를 할당 받아 데이터를 채움. 
    // 패턴은 4 바이트의 LBA 어드레스와 4 바이트의 쓰기가 수행된 횟수로 생성
    pcBuffer = kAllocateMemory( iSectorCount * 512 );
    for( j = 0 ; j < iSectorCount ; j++ )
    {
        for( i = 0 ; i < 512 ; i += 8 )
        {
            *( DWORD* ) &( pcBuffer[ j * 512 + i ] ) = dwLBA + j;
            *( DWORD* ) &( pcBuffer[ j * 512 + i + 4 ] ) = s_dwWriteCount;            
        }
    }
    
    // 쓰기 수행
    if( kWriteHDDSector( TRUE, TRUE, dwLBA, iSectorCount, pcBuffer ) != iSectorCount )
    {
        kPrintf( "Write Fail\n" );
        return ;
    }
    kPrintf( "LBA [%d], [%d] Sector Write Success~!!", dwLBA, iSectorCount );

    // 데이터 버퍼의 내용을 출력
    for( j = 0 ; j < iSectorCount ; j++ )
    {
        for( i = 0 ; i < 512 ; i++ )
        {
            if( !( ( j == 0 ) && ( i == 0 ) ) && ( ( i % 256 ) == 0 ) )
            {
                kPrintf( "\nPress any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    bExit = TRUE;
                    break;
                }
            }                

            if( ( i % 16 ) == 0 )
            {
                kPrintf( "\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i ); 
            }

            // 모두 두 자리로 표시하려고 16보다 작은 경우 0을 추가해줌
            bData = pcBuffer[ j * 512 + i ] & 0xFF;
            if( bData < 16 )
            {
                kPrintf( "0" );
            }
            kPrintf( "%X ", bData );
        }
        
        if( bExit == TRUE )
        {
            break;
        }
    }
    kPrintf( "\n" );    
    kFreeMemory( pcBuffer );    
}