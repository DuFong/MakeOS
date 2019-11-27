#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "Types.h"

#define SHIFT_TABLE             12
#define SHIFT_DIRECTORY         21
#define SHIFT_DIRECTORYPTR      30
#define SHIFT_PML4              39
#define MASK_OFFSET             0xFFF
#define MASK_ENTRYOFFSET        0x1FF
#define SIZE_ENTRY              8
#define MASK_BASEADDRESS        0xFFFF000

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode);
void kPageFaultHandler(int iVectorNumber, QWORD qwErrorCode);
void kCommonInterruptHandler(int iVectorNumber);
void kKeyboardHandler(int iVectorNumber);
void kTimerHandler(int iVectorNumber);
void kHDDHandler( int iVectorNumber );

#endif /*__INTERRUPTHANDLER_H__*/