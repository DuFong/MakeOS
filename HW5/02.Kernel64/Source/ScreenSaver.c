#include "ScreenSaver.h"
#include "Task.h"

SCREENSAVER g_stScreenSaver;

void kInitializeScreenSaver(){
    g_stScreenSaver.qwScreenOnCount = 0;
    // 2분 후에 화면보호기가 켜지도록 설정
    g_stScreenSaver.qwRestrictionScreenOn = 60000;
    g_stScreenSaver.bScreenSaverOn = FALSE;
    g_stScreenSaver.pstProcess = NULL;
}