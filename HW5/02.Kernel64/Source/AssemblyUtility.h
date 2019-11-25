#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"
#include "Task.h"

BYTE kInPortByte(WORD wPort);
void kOutPortByte(WORD wPort, BYTE bData);
void kLoadGDTR(QWORD qwGDTRAddress);
void kLoadTR(WORD wTSSSegmentOffset);
void kLoadIDTR(QWORD qwIDTRAddress);
void kEnableInterrupt();
void kDisableInterrupt();
QWORD kReadRFLAGS();
QWORD kGetExceptionAddress();
QWORD kGetPML4BaseAddress();
QWORD kReadTSC();
void kSwitchContext(CONTEXT* pstCurrentContext, CONTEXT* pstNextContext);
void kHlt(void);
BOOL kTestAndSet(volatile BYTE* pbDestination, BYTE bCompare, BYTE bSource);

#endif /*__ASSEMBLYUTILITY_H__*/