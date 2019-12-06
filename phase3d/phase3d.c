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

static int clockHand = 0; 	//the position of the clock hand;
static int clockAlg(int pid) ;
int semClockHand;	//protect clock
static int swapInitialized = FALSE;

static void debug3(char *fmt, ...)
{
    va_list ap;

    if (debugging3) {
        va_start(ap, fmt);
        USLOSS_VConsole(fmt, ap);
    }
}

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
	P1_SemCreate("clockHand",1,&semClockHand);
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
	if(swapInitialized==FALSE){
        	return P3_NOT_INITIALIZED;
    	}
    int result = P1_SUCCESS;

    // clean things up
	P1_SemFree(semClockHand);
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
	if(swapInitialized==FALSE){
        	return P3_NOT_INITIALIZED;
    	}
    int result = P1_SUCCESS;

    /*****************

    P(mutex)
    free all swap space used by the process
    V(mutex)

    *****************/


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
	if(swapInitialized==FALSE){
        	return P3_NOT_INITIALIZED;
    	}
	int freeFrame = -1;		//shouldn't return -1
	int access;
	int pagePtr;
 	USLOSS_PTE *table;
 	char buffer[USLOSS_MmuPageSize()];
 	P3PageTableGet(pid,&table);
	// clock alg
	P1_P(semClockHand);
 	while(freeFrame == -1) {
		result = USLOSS_MmuGetAccess(clockPos, &access);
		if ((access & USLOSS_MMU_REF) == 0) 
			freeFrame = clockHand;
		else 
			USLOSS_MmuSetAccess(clockHand, access & 0x2);
        clockHand = (clockHand+1)%P3_vmStats.frames;
	}
	int vmRegion=USLOSS_MmuRegion(&pagePtr);
	            if(access == USLOSS_MMU_DIRTY){//    if frame[target] is dirty (USLOSS_MmuGetAccess)
			memcpy(&buffer, vmRegion+freeFrame*USLOSS_MmuPageSize() , USLOSS_MmuPageSize());
			P2_DiskWrite
	      //  write page to its location on the swap disk (P3FrameMap,P2_DiskWrite,P3FrameUnmap)
			    //P3_SWAP_DISK
      		//  clear dirty bit (USLOSS_MmuSetAccess)
   		// update page table of process to indicate page is no longer in a frame
   		// mark frames[target] as busy
		    }
	P1_V(semClockHand);
	return P1_SUCCESS;
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
	if(swapInitialized==FALSE){
        	return P3_NOT_INITIALIZED;
    	}
    int result = P1_SUCCESS;

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

    return result;
}
