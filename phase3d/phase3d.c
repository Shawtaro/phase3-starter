/*
 * phase3d.c
 *
 */

/***************

NOTES ON SYNCHRONIZATION

There are various shared resources that require proper synchronization. 

Swap space. Free swap space is a shared resource, we don't want multiple pagers choosing the
same free space to hold a page. You'll need a mutex around the free swap space.

The clock hand is also a shared resource.

The frames are a shared resource in that we don't want multiple pagers to choose the same frame via
the clock algorithm. That's the purpose of marking a frame as "busy" in the pseudo-code below. 
Pagers ignore busy frames when running the clock algorithm.

A process's page table is a shared resource with the pager. The process changes its page table
when it quits, and a pager changes the page table when it selects one of the process's pages
in the clock algorithm. 

Normally the pagers would perform I/O concurrently, which means they would release the mutex
while performing disk I/O. I made it simpler by having the pagers hold the mutex while they perform
disk I/O.

***************/


#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <string.h>
#include <libuser.h>

#include "phase3.h"
#include "phase3Int.h"

#ifdef DEBUG
static int debugging3 = 1;
#else
static int debugging3 = 0;
#endif

static void debug3(char *fmt, ...)
{
    va_list ap;

    if (debugging3) {
        va_start(ap, fmt);
        USLOSS_VConsole(fmt, ap);
    }
}


static int initialized=FALSE;
static int numFrames;
static int numPages;
static int * framesTable;
static int mutex;

typedef struct Data{
    int pid;
    int frame;
    int first;
    int track;
    int page;
}Data;

static int sectorSize;
static int trackSize;
static int tracks;
static int sectors;
static int start;
static Data *swapData;
void *addr = NULL;
/*
 *----------------------------------------------------------------------
 *
 * P3SwapInit --
 *
 *  Initializes the swap data structures.
 *
 * Results:
 *   P3_ALREADY_INITIALIZED:    this function has already been called
 *   P1_SUCCESS:                success
 *
 *----------------------------------------------------------------------
 */
int
P3SwapInit(int pages, int frames)
{
    int result = P1_SUCCESS;

    // initialize the swap data structures, e.g. the pool of free blocks
    if(initialized==TRUE){
        return P3_ALREADY_INITIALIZED;
    }
    result = P1_SemCreate("Mutex",1,&mutex);
    numFrames = frames;
    numPages = pages;
    framesTable=malloc(sizeof(int)*numFrames);
    for(int i=0;i<numFrames;i++){
        framesTable[i]=FALSE;
    }
    result = P2_DiskSize(P3_SWAP_DISK,&sectorSize,&trackSize,&tracks);
    sectors = tracks*trackSize;
    swapData = malloc(sizeof(Data)*sectors);
    for(int i=0;i<sectors;i++){
        swapData[i].pid=-1;
        swapData[i].frame=-1;
        swapData[i].first=i%tracks;
        swapData[i].track=i/tracks;
        swapData[i].page= -1;
    }
    initialized=TRUE;
    start = 0;
    return result;
}
/*
 *----------------------------------------------------------------------
 *
 * P3SwapShutdown --
 *
 *  Cleans up the swap data structures.
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3SwapInit has not been called
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */
int
P3SwapShutdown(void)
{
    if( initialized == FALSE){
        return P3_NOT_INITIALIZED;
    }
    int result = P1_SUCCESS;

    // clean things up
    free(swapData);
    result = P1_SemFree(mutex);
    initialized = FALSE;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * P3SwapFreeAll --
 *
 *  Frees all swap space used by a process
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3SwapInit has not been called
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */

int
P3SwapFreeAll(int pid)
{
    int result = P1_SUCCESS;

    /*****************

    P(mutex)
    free all swap space used by the process
    V(mutex)

    *****************/
    result = P1_P(mutex);
    //free all swap space used by the process
    for(int i=0;i<sectors;i++){
        if(swapData[i].pid==pid){
            framesTable[swapData[i].frame]=FALSE;
            result = P2_DiskWrite(P3_SWAP_DISK,swapData[i].track,swapData[i].first,1,NULL);
            swapData[i].pid=-1;
            swapData[i].frame=-1;
            swapData[i].page= -1;
        }
    }
    result = P1_V(mutex);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * P3SwapOut --
 *
 * Uses the clock algorithm to select a frame to replace, writing the page that is in the frame out 
 * to swap if it is dirty. The page table of the page’s process is modified so that the page no 
 * longer maps to the frame. The frame that was selected is returned in *frame. 
 *
 * Results:
 *   P3_NOT_INITIALIZED:    P3SwapInit has not been called
 *   P1_SUCCESS:            success
 *
 *----------------------------------------------------------------------
 */
int
P3SwapOut(int *frame) 
{
    if(initialized==FALSE){
        return P3_NOT_INITIALIZED;
    }
    int result = P1_SUCCESS;

    /*****************

    NOTE: in the pseudo-code below I used the notation frames[x] to indicate frame x. You 
    may or may not have an actual array with this name. As with all my pseudo-code feel free
    to ignore it.


    static int hand = -1;    // start with frame 0
    P(mutex)
    loop
        hand = (hand + 1) % # of frames
        if frames[hand] is not busy
            if frames[hand] hasn't been referenced (USLOSS_MmuGetAccess)
                target = hand
                break
            else
                clear reference bit (USLOSS_MmuSetAccess)
    if frame[target] is dirty (USLOSS_MmuGetAccess)
        write page to its location on the swap disk (P3FrameMap,P2_DiskWrite,P3FrameUnmap)
        clear dirty bit (USLOSS_MmuSetAccess)
    update page table of process to indicate page is no longer in a frame
    mark frames[target] as busy
    V(mutex)
    *frame = target

    *****************/
    static int hand = -1;
    result = P1_P(mutex);
    int target;
    int accessPtr;
    while(1){
        hand = (hand+1)%numFrames;
        if(framesTable[hand]==FALSE){
            result = USLOSS_MmuGetAccess(hand,&accessPtr);
            if((accessPtr&1)!=USLOSS_MMU_REF){
                target = hand;
                break;
            }else{
                result = USLOSS_MmuSetAccess(hand,accessPtr&2);
            }
        }
    }
    int index;
    for(index=0;index<sectors;index++){
        if(swapData[index].frame==target){
            break;
        }
    }
    if((accessPtr&2)==USLOSS_MMU_DIRTY){    
        //void *addr;     
        result = P3FrameMap(target,&addr);
        
        // write page to its location on the swap disk
        void *buffer=malloc(USLOSS_MmuPageSize());
        memcpy(buffer,addr,USLOSS_MmuPageSize());
        result = P2_DiskWrite(P3_SWAP_DISK,swapData[index].track,swapData[index].first,USLOSS_MmuPageSize()/sectorSize,buffer);
        free(buffer);

        result = P3FrameUnmap(target);
        result = USLOSS_MmuSetAccess(target,accessPtr&1);
    }
    
    // update page table of process to indicate page is no longer in a frame
    USLOSS_PTE  *table = NULL;
    result = P3PageTableGet(swapData[index].pid,&table);
    (table+swapData[index].page)->incore=0;
    (table+swapData[index].page)->frame=-1;

    result = USLOSS_MmuSetPageTable(table); 
    framesTable[hand]=TRUE;
    result = P1_V(mutex);
    *frame=target;
    return result;
}
/*
 *----------------------------------------------------------------------
 *
 * P3SwapIn --
 *
 *  Opposite of P3FrameMap. The frame is unmapped.
 *
 * Results:
 *   P3_NOT_INITIALIZED:     P3SwapInit has not been called
 *   P1_INVALID_PID:         pid is invalid      
 *   P1_INVALID_PAGE:        page is invalid         
 *   P1_INVALID_FRAME:       frame is invalid
 *   P1_OUT_OF_SWAP:         there is no more swap space
 *   P1_SUCCESS:             success
 *
 *----------------------------------------------------------------------
 */
int
P3SwapIn(int pid, int page, int frame)
{
    if(initialized==FALSE){
        return P3_NOT_INITIALIZED;
    }
    if(pid<0||pid>=P1_MAXPROC){
        return P1_INVALID_PID;
    }
    if(page<0||page>=numPages){
        return P3_OUT_OF_PAGES;
    }
    if(frame<0||frame>=numFrames){
        return P3_INVALID_FRAME;
    }

    /*****************

    P(mutex)
    if page is on swap disk
        read page from swap disk into frame (P3FrameMap,P2_DiskRead,P3FrameUnmap)
    else
        allocate space for the page on the swap disk
        if no more space
            result = P3_OUT_OF_SWAP
        else
            result = P3_EMPTY_PAGE
    mark frame as not busy
    V(mutex)

    *****************/
    int result = P1_SUCCESS;
    result = P1_P(mutex);
    int onDisk = FALSE;
    int index;
    for(index=0;index<sectors;index++){
        if(swapData[index].pid==pid&&swapData[index].page==page){
            onDisk=TRUE;
            break;
        }
    }
    if(onDisk==TRUE){
        result = P3FrameMap(frame,&addr);
        void *buffer=malloc(USLOSS_MmuPageSize());
        result = P2_DiskRead(P3_SWAP_DISK,swapData[index].track,swapData[index].first,USLOSS_MmuPageSize()/sectorSize, buffer);
        memcpy(addr,buffer,USLOSS_MmuPageSize());
        free(buffer);
        result = P3FrameUnmap(frame);
    }else{
        for(index=0;index<sectors;index++){
            if(swapData[index].pid==-1){
                swapData[index].pid = pid;
                swapData[index].frame = frame;
                swapData[index].page = page;
                break;
            }
        }
        if(index == sectors){
            framesTable[frame]=FALSE;
            result = P1_V(mutex);
            return P3_OUT_OF_SWAP;
        }else{
            framesTable[frame]=FALSE;
            result = P1_V(mutex);
            return P3_EMPTY_PAGE;
        }
    }
    USLOSS_PTE  *table = NULL;
    result = P3PageTableGet(swapData[index].pid,&table);
    (table+page)->incore=1;
    (table+page)->write=1;
    (table+page)->read=1;
    (table+page)->frame=frame;
    result = USLOSS_MmuSetPageTable(table);
    framesTable[frame]=FALSE;
    result = P1_V(mutex);
    return result;
}