/*
 * phase3c.c
 *
 */

#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <string.h>
#include <libuser.h>

#include "phase3.h"
#include "phase3Int.h"

#ifdef DEBUG
int debugging3 = 1;
#else
int debugging3 = 0;
#endif

void debug3(char *fmt, ...)
{
    va_list ap;

    if (debugging3) {
        va_start(ap, fmt);
        USLOSS_VConsole(fmt, ap);
    }
}

static int frameInitialized = FALSE;
static int pagerInitialized = FALSE;
static int  numPages = 0; // # of pages in a page table
static int numFrames = 0; // # of frames in physical memory

typedef struct Frame{
    int id;
    int used;
}Frame;
static Frame * framesTable = NULL;

// information about a fault. Add to this as necessary.

typedef struct Fault {
    PID         pid;
    int         offset;
    int         cause;
    SID         wait;
    // other stuff goes here
    int         rc;
} Fault;

static Fault faultQueue[P1_MAXPROC];
static int qFront = 0;
static int qRear = 0;

static int numPagers;
static int *pagerPID;
static int Pager(void *arg);
static int pagerRunning;
static int pagerMutex;
static int faultMutex;
static int pagerShutdown=FALSE;

/*
 *----------------------------------------------------------------------
 *
 * P3FrameInit --
 *
 *  Initializes the frame data structures.
 *
 * Results:
 *   P3_ALREADY_INITIALIZED:    this function has already been called
 *   P1_SUCCESS:                success
 *
 *----------------------------------------------------------------------
 */
int
P3FrameInit(int pages, int frames)
{
    int result = P1_SUCCESS;
    if(frameInitialized==TRUE){
        return P3_ALREADY_INITIALIZED;
    }
    // initialize the frame data structures, e.g. the pool of free frames
    // set P3_vmStats.freeFrames
    numPages=pages;
    numFrames=frames;
    framesTable = malloc(sizeof(Frame)*numFrames);
    for(int i=0;i<frames;i++){
        framesTable[i].id=i;
        framesTable[i].used=FALSE;
    }
    P3_vmStats.freeFrames = frames;
    frameInitialized = TRUE;
    return result;
}
/*
 *----------------------------------------------------------------------
 *
 * P3FrameShutdown --
 *
 *  Cleans up the frame data structures.
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3FrameInit has not been called
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */
int
P3FrameShutdown(void)
{
    // check kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    int result = P1_SUCCESS;
    if(frameInitialized==FALSE){
        return P3_NOT_INITIALIZED;
    }
    // clean things up
    free(framesTable);
    frameInitialized = FALSE;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * P3FrameFreeAll --
 *
 *  Frees all frames used by a process
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3FrameInit has not been called
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */

int
P3FrameFreeAll(int pid)
{
        // check kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    int result = P1_SUCCESS;
    if(frameInitialized==FALSE){
        return P3_NOT_INITIALIZED;
    }
    // free all frames in use by the process (P3PageTableGet)
    USLOSS_PTE  *table = NULL;
    result = P3PageTableGet(pid,&table);
    if(result==P1_SUCCESS){
        for(int i=0;i<numPages;i++){
            framesTable[table[i].frame].used=FALSE;
        }
    } else
        return P1_INVALID_PIDl
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * P3FrameMap --
 *
 *  Maps a frame to an unused page and returns a pointer to it.
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3FrameInit has not been called
 *   P3_OUT_OF_PAGES:       process has no free pages
 *   P1_INVALID_FRAME       the frame number is invalid
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */
int
P3FrameMap(int frame, void **addr) 
{
        // check kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    int result = P1_SUCCESS;
    if(frameInitialized==FALSE){
        return P3_NOT_INITIALIZED;
    }
    // get the page table for the process (P3PageTableGet)
    // find an unused page
    // update the page's PTE to map the page to the frame
    // update the page table in the MMU (USLOSS_MmuSetPageTable)
    int pid = faultQueue[qFront].pid;
    int page;
    printf("page: %d\n", faultQueue[qFront].offset);
    USLOSS_PTE  *table = NULL;
    result = P3PageTableGet(pid,&table);
    for(page=0;page<numPages;page++){
        if((table+page)->incore==0){
            break;
        }
    }
    if(page<0||page>=numPages){
        return P3_OUT_OF_PAGES;
    }
    if(frame<0||frame>=numFrames){
        return P3_INVALID_FRAME;
    }
    (table+page)->incore=1;
    (table+page)->read=1;
    (table+page)->write=1;
    (table+page)->frame=frame;
    framesTable[frame].used = TRUE;
    printf("map pid:%d page:%d frame:%d\n", pid,page,frame);
    result = USLOSS_MmuSetPageTable(table);
    *addr = (void*) (table+page);
    return result;
}
/*
 *----------------------------------------------------------------------
 *
 * P3FrameUnmap --
 *
 *  Opposite of P3FrameMap. The frame is unmapped.
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3FrameInit has not been called
 *   P3_FRAME_NOT_MAPPED:   process didn’t map frame via P3FrameMap
 *   P1_INVALID_FRAME       the frame number is invalid
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */
int
P3FrameUnmap(int frame) 
{
        // check kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    int result = P1_SUCCESS;
    if(frameInitialized==FALSE){
        return P3_NOT_INITIALIZED;
    }
    // get the process's page table (P3PageTableGet)
    // verify that the process mapped the frame
    // update page's PTE to remove the mapping
    // update the page table in the MMU (USLOSS_MmuSetPageTable)
    int pid = faultQueue[qFront].pid;
    int page = faultQueue[qFront].offset/USLOSS_MmuPageSize();
    USLOSS_PTE  *table = NULL;
    result = P3PageTableGet(pid,&table);
    if(page<0||page>=numPages){
        return P3_FRAME_NOT_MAPPED;
    }
    if(frame<0||frame>=numFrames){
        return P3_INVALID_FRAME;
    }
    printf("unmap pid:%d page:%d frame:%d\n", pid,page,frame);
    result = USLOSS_MmuSetPageTable(table);
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * FaultHandler --
 *
 *  Page fault interrupt handler
 *
 *----------------------------------------------------------------------
 */

static void
FaultHandler(int type, void *arg)
{
    Fault   fault;
    int result;
    fault.offset = (int) arg;
    // fill in other fields in fault
    fault.pid=P1_GetPid();
    fault.rc=0;
    printf("fault %d\n", fault.pid);
    fault.cause=USLOSS_MmuGetCause();
    char name[P1_MAXNAME+1];
    snprintf(name, sizeof(name), "Fault %d", fault.pid);
    result = P1_SemCreate(name,0,&fault.wait);
    // add to queue of pending faults
    faultQueue[qRear]=fault;
    qRear=(qRear+1)%P1_MAXPROC;
    // let pagers know there is a pending fault
    result = P1_V(faultMutex);
    // wait for fault to be handled
    result = P1_P(fault.wait);
    result = P1_SemFree(fault.wait);
    if(faultQueue[qFront].rc==USLOSS_MMU_ACCESS){
        P2_Terminate(USLOSS_MMU_ACCESS);
    }else if(faultQueue[qFront].rc==P3_OUT_OF_SWAP){
        P2_Terminate(P3_OUT_OF_SWAP);
    }
    qFront=(qFront+1)%P1_MAXPROC;
}


/*
 *----------------------------------------------------------------------
 *
 * P3PagerInit --
 *
 *  Initializes the pagers.
 *
 * Results:
 *   P3_ALREADY_INITIALIZED: this function has already been called
 *   P3_INVALID_NUM_PAGERS:  the number of pagers is invalid
 *   P1_SUCCESS:             success
 *
 *----------------------------------------------------------------------
 */
int
P3PagerInit(int pages, int frames, int pagers)
{
        // check kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    int     result = P1_SUCCESS;

    USLOSS_IntVec[USLOSS_MMU_INT] = FaultHandler;
    if(pagerInitialized==TRUE){
        return P3_ALREADY_INITIALIZED;
    }
    // initialize the pager data structures
    // fork off the pagers and wait for them to start running
    if(pagers<0||pagers>P3_MAX_PAGERS){
        return P3_INVALID_NUM_PAGERS;
    }
    pagerPID=malloc(sizeof(int)*pagers);
    result = P1_SemCreate("faultMutex",0,&faultMutex);
    result = P1_SemCreate("pagerMutex",1,&pagerMutex);
    result = P1_SemCreate("pagerRunning",0,&pagerRunning);
    numPagers = pagers;
    for(int i=0;i<pagers;i++){
        char name[P1_MAXNAME+1];
        snprintf(name, sizeof(name), "Pager %d", i);
        result = P1_Fork(name,Pager,NULL,USLOSS_MIN_STACK * 4,P3_PAGER_PRIORITY,0,&pagerPID[i]);
        result = P1_P(pagerRunning);
    }
    pagerInitialized=TRUE;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * P3PagerShutdown --
 *
 *  Kills the pagers and cleans up.
 *
 * Results:
 *   P3_NOT_INITIALIZED:     P3PagerInit has not been called
 *   P1_SUCCESS:             success
 *
 *----------------------------------------------------------------------
 */
int
P3PagerShutdown(void)
{
        // check kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    int result = P1_SUCCESS;
    if(pagerInitialized==FALSE){
        return P3_NOT_INITIALIZED;
    }
    // cause the pagers to quit
    pagerInitialized=FALSE;
    pagerShutdown=TRUE;
    for(int i=0;i<numPagers;i++){
        result = P1_V(faultMutex);
    }
    // clean up the pager data structures
    free(pagerPID);
    result = P1_SemFree(faultMutex);
    result = P1_SemFree(pagerMutex);
    result = P1_SemFree(pagerRunning);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Pager --
 *
 *  Handles page faults
 *
 *----------------------------------------------------------------------
 */

static int
Pager(void *arg)
{
    /********************************

    notify P3PagerInit that we are running
    loop until P3PagerShutdown is called
        wait for a fault
        if it's an access fault kill the faulting process
        if there are free frames
            frame = a free frame
        else
            P3SwapOut(&frame);
        rc = P3SwapIn(pid, page, frame)
        if rc == P3_EMPTY_PAGE
            P3FrameMap(frame, &addr)
            zero-out frame at addr
            P3FrameUnmap(frame);
        else if rc == P3_OUT_OF_SWAP
            kill the faulting process
        update PTE in faulting process's page table to map page to frame
        unblock faulting process

    **********************************/
    int result = P1_V(pagerRunning);
    while(1){
        result = P1_P(faultMutex);
        if(pagerShutdown==TRUE){
            break;
        }
        result = P1_P(pagerMutex);
        Fault fault = faultQueue[qFront];
        if(fault.cause==USLOSS_MMU_ACCESS){
            faultQueue[qFront].rc = USLOSS_MMU_ACCESS;
            result = P1_V(fault.wait);
            result = P1_V(pagerMutex);
            continue;
        }
        int frame = 0;
        for(;frame<numFrames;frame++){
            if(framesTable[frame].used==FALSE){
                break;
            }
        }
        if(frame==numFrames){
            result = P3SwapOut(&frame);
        }
        int page = fault.offset/USLOSS_MmuPageSize();
        result = P3SwapIn(fault.pid, page, frame);
        if (result == P3_EMPTY_PAGE){
            
        }else if (result == P3_OUT_OF_SWAP){
            faultQueue[qFront].rc = P3_OUT_OF_SWAP;
            result = P1_V(fault.wait);
            result = P1_V(pagerMutex);
            continue;
        }
        USLOSS_PTE *table = NULL;
        result = P3PageTableGet(fault.pid,&table);
        
        result = P1_V(fault.wait);
        result = P1_V(pagerMutex);
    }
    return result;
}
