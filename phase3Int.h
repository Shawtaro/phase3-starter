/*
 * Internal definitions for Phase 3 of the project (virtual memory).
 */
#ifndef _PHASE3_INT_H
#define _PHASE3_INT_H

#include <phase3.h>

#ifndef CHECKRETURN
#define CHECKRETURN __attribute__((warn_unused_result))
#endif

// useful definitions

typedef int SID;    // semaphore ID
typedef int PID;    // PID

// helpful macro
#define CheckMode() \
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) { \
        int pid; \
        Sys_GetPID(&pid); \
        USLOSS_Console("Process %d called %s from user mode.\n", pid, __FUNCTION__); \
        USLOSS_IllegalInstruction(); \
    }

// Phase 3a

int         P3PageTableGet(PID pid, USLOSS_PTE **table) CHECKRETURN;
int         P3PageTableSet(PID pid, USLOSS_PTE *table) CHECKRETURN;


// Phase 3b

void        P3PageFaultHandler(int type, void *arg);
USLOSS_PTE *P3PageTableAllocateEmpty(int numPages) CHECKRETURN;

// Phase 3c

int         P3FrameInit(int pages, int frames) CHECKRETURN;
int         P3FrameShutdown(void) CHECKRETURN;
int         P3FrameFreeAll(PID pid) CHECKRETURN;
int         P3FrameMap(int frame, void **addr) CHECKRETURN;
int         P3FrameUnmap(int frame) CHECKRETURN;

int         P3PagerInit(int pages, int frames, int pagers) CHECKRETURN;
int         P3PagerShutdown(void)  CHECKRETURN;

// Phase 3d

int         P3SwapInit(int pages, int frames) CHECKRETURN;
int         P3SwapShutdown(void) CHECKRETURN;
int         P3SwapFreeAll(PID pid) CHECKRETURN;
int         P3SwapOut(int *frame) CHECKRETURN;
int         P3SwapIn(PID pid, int page, int frame) CHECKRETURN;

#endif
