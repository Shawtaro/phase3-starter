/*
 * Definitions for Phase 3 of the project (virtual memory).
 */
#ifndef _PHASE3_H
#define _PHASE3_H

#include <usloss.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Maximum number of pager processes.
 */
#define P3_MAX_PAGERS   3

/*
 * Pager priority.
 */
#define P3_PAGER_PRIORITY   2

/*
 * Swap disk.
 */
#define P3_SWAP_DISK 1

/*
 * Paging statistics
 */
typedef struct P3_VmStats {
    int pages;      /* Size of VM region, in pages */
    int frames;     /* Size of physical memory, in frames */
    int blocks;     /* Size of disk, in blocks (pages) */
    int freeFrames; /* # of frames that are not in-use */
    int freeBlocks; /* # of blocks that are not in-use */
    int faults;     /* # of page faults */
    int new;        /* # faults caused by previously unused pages*/
    int pageIns;    /* # faults that required reading page from disk */
    int pageOuts;   /* # faults that required writing a page to disk */
    int replaced;   /* # pages replaced */
} P3_VmStats;

extern P3_VmStats P3_vmStats;

/*
 * Error codes
 */

#define P3_INVALID_NUM_PAGES        -30
#define P3_INVALID_NUM_FRAMES       -31
#define P3_INVALID_NUM_PAGERS       -32
#define P3_ALREADY_INITIALIZED      -33
#define P3_HAS_TABLE                -34
#define P3_FRAME_NOT_MAPPED         -35
#define P3_EMPTY_PAGE               -36
#define P3_OUT_OF_SWAP              -37
#define P3_NOT_INITIALIZED          -38
#define P3_OUT_OF_PAGES             -39
#define P3_INVALID_FRAME            -40

#ifndef CHECKRETURN
#define CHECKRETURN __attribute__((warn_unused_result))
#endif

extern int          P3_VmInit(int mappings, int pages, int frames, int pagers) CHECKRETURN;
extern void         P3_VmDestroy(void);
extern  USLOSS_PTE  *P3_AllocatePageTable(int pid) CHECKRETURN;
extern  void        P3_FreePageTable(int pid);
extern void         P3_PrintStats(P3_VmStats *stats);

extern int  P4_Startup(void *) CHECKRETURN;

#endif
