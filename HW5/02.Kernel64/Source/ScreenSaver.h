#ifndef __SCREENSAVER_H__
#define __SCREENSAVER_H__

#include "Types.h"
#include "Console.h"
#include "Task.h"

#pragma pack(push, 1)

typedef struct kCursorPoint{
    int iX;
    int iY;
} CURSORPOINT;

typedef struct kScreenSaverStruct{
    // 마지막으로 외부 디바이스 인터럽트가 발생한 후 지난 시간
    volatile QWORD qwScreenOnCount;
    // 아무런 작업이 없을 때 화면보호기가 켜지기까지 시간
    QWORD qwRestrictionScreenOn;
    // 현재 화면보호기 작동중 여부
    BOOL bScreenSaverOn;
    // 화면보호기가 실행되기 전 화면을 저장
    CHARACTER vcScreen[CONSOLE_WIDTH * CONSOLE_HEIGHT];
    // 화면보호기가 실행되기 전 커서 위치 저장
    CURSORPOINT stCursor;
    // 화면보호기 프로세스
    TCB* pstProcess;

} SCREENSAVER;

#pragma pack(pop)

void kInitializeScreenSaver();

extern SCREENSAVER g_stScreenSaver;

#endif /*__SCREENSAVER_H__*/