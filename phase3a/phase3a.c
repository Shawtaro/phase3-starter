/*
 * phase3a.c
 *
 * 	This is a skeleton for phase3a.
 */

#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <string.h>
#include <libuser.h>

#include "phase3Int.h"

static USLOSS_PTE   *pageTables[P1_MAXPROC];
static int	numPages = 0; // # of pages in a page table
static int numFrames = 0; // # of frames in physical memory

P3_VmStats	P3_vmStats;

static USLOSS_PTE  *PageTableAllocateIdentity(int pages);

static int initialized = FALSE;

static int          MMUInit(int pages, int frames);
static int          MMUShutdown(void);
static int          PageTableFree(PID pid);


/*
 *----------------------------------------------------------------------
 *
 * P3_VmInit --
 *
 *	Initializes the VM system by configuring the MMU and setting
 *	up the page tables.
 *
 * Parameters:
 *      mappings: unused
 *      pages: # of pages in the VM region
 *      frames: # of frames of physical memory
 *      pagers: # of pager daemons
 *	
 * Results:
 *      TBD
 *
 * Side effects:
 *      The MMU is initialized.
 *
 *----------------------------------------------------------------------
 */
int
P3_VmInit(int unused, int pages, int frames, int pagers)
{
    int     result = P1_SUCCESS;

    CheckMode();

    if (initialized) {
        result = P3_ALREADY_INITIALIZED;
        goto done;
    }

    if ((pagers < 0) || (pagers > P3_MAX_PAGERS)) {
        result = P3_INVALID_NUM_PAGERS;
        goto done;
    }

    for (int i = 0; i < P1_MAXPROC; i++) {
        pageTables[i] = NULL;
    }

    USLOSS_IntVec[USLOSS_MMU_INT] = P3PageFaultHandler;

    result = MMUInit(pages, frames);
    if (result != P1_SUCCESS) {
        USLOSS_Console("MMUInit failed: %d\n", result);
        goto done;
    }

    result = P3FrameInit(frames);
    if (result != P1_SUCCESS) {
        USLOSS_Console("P3FrameInit failed: %d\n", result);
        goto done;
    }

    result = P3PagerInit(pagers);
    if (result != P1_SUCCESS) {
        USLOSS_Console("P3PagerInit failed: %d\n", result);
        goto done;
    }

    result = P3SwapInit(pages, frames);
    if (result != P1_SUCCESS) {
        USLOSS_Console("P3SwapInit failed: %d\n", result);
        goto done;
    }

    numPages = pages;
    numFrames = frames;
    memset((char *) &P3_vmStats, 0, sizeof(P3_vmStats));
    P3_vmStats.pages = pages;
    P3_vmStats.frames = frames;
    initialized = TRUE;
    result = P1_SUCCESS;
done:
    return result;
}
/*
 *----------------------------------------------------------------------
 *
 * P3_VmShutdown --
 *
 *	Shut it all down.
 *	
 * Results:
 *      None
 *
 * Side effects:
 *      Everything VM-related is cleaned up.
 *
 *----------------------------------------------------------------------
 */
void
P3_VmShutdown(void)
{
    int rc;

    CheckMode();
    if (initialized) {

        rc = P3SwapShutdown();
        assert(rc == P1_SUCCESS);

        rc = P3PagerShutdown();
        assert(rc == P1_SUCCESS);

        rc = P3FrameShutdown();
        assert(rc == P1_SUCCESS);

        rc = MMUShutdown();
        assert(rc == P1_SUCCESS);

        for (int i = 0; i < P1_MAXPROC; i++) {
            if (pageTables[i] != NULL) {
                rc = PageTableFree(i);
                assert(rc == P1_SUCCESS);
                pageTables[i] = NULL;
            }
        }

        P3_PrintStats(&P3_vmStats);
        initialized = FALSE;      
    }
}

/*
 *----------------------------------------------------------------------
 *
 * P3_AllocatePageTable --
 *
 *	Allocates a page table for the new process.
 *
 * Parameters:
 *      pid : pid of new process
 *
 * Results:
 *	 Page table.
 *
 * Side effects:
 *	 A page table is allocated.
 *
 *----------------------------------------------------------------------
 */
USLOSS_PTE *
P3_AllocatePageTable(int pid)
{
    USLOSS_PTE  *pageTable = NULL;

    CheckMode();
    if ((pid < 0) || (pid >= P1_MAXPROC)) {
        USLOSS_Console("P3_AllocatePageTable: invalid pid %d\n", pid);
        goto done;
    }
    if (initialized) {
        pageTable = P3PageTableAllocateEmpty(numPages);
        if (pageTable == NULL) {
            pageTable = PageTableAllocateIdentity(numPages);
        }
        pageTables[pid] = pageTable;
    }
done:
    return pageTable;
}

/*
 *----------------------------------------------------------------------
 *
 * P3_FreePageTable --
 *
 *	Called when a process quits and frees the page table 
 *	for the process and frees any frames and disk space used
 *  by the process.
 *
 * Parameters:
 *      pid: pid of process that is quitting
 *
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
P3_FreePageTable(int pid)
{
    int rc;

    CheckMode();
    if ((pid < 0) || (pid >= P1_MAXPROC)) {
        USLOSS_Console("P3_FreePageTable: invalid pid %d\n", pid);
        goto done;
    }
    if ((initialized) && (pageTables[pid] != NULL)) {

        rc = P3FrameFreeAll(pid);
        if (rc != P1_SUCCESS) {
            USLOSS_Console("P3_FreePageTable: P3FrameFreeAll(%d) failed: %d\n", pid, rc);
            goto done;
        }

        rc = P3SwapFreeAll(pid);
        if (rc != P1_SUCCESS) {
            USLOSS_Console("P3_FreePageTable: P3SwapFreeAll(%d) failed: %d\n", pid, rc);
            goto done;
        }

        rc = PageTableFree(pid);
        if (rc != P1_SUCCESS) {
            USLOSS_Console("P3_FreePageTable: PageTableFree(%d) failed: %d\n", pid, rc);
            goto done;
        }
    }
done:
    return;
}

int
P3PageTableGet(PID pid, USLOSS_PTE **table)
{
    int result = P1_SUCCESS;
    if ((pid < 0) || (pid >= P1_MAXPROC)) {
        result = P1_INVALID_PID;
    } else {
        *table = pageTables[pid];
    }
    return result;
}

int
P3PageTableSet(PID pid, USLOSS_PTE *table)
{
    int result = P1_SUCCESS;
    if ((pid < 0) || (pid >= P1_MAXPROC)) {
        result = P1_INVALID_PID;
    } else {
        pageTables[pid] = table;
    }
    return result;
}

static int
MMUInit(int pages, int frames) 
{
    int rc;
    int result = P1_SUCCESS;

    rc = USLOSS_MmuInit(pages, pages, frames, USLOSS_MMU_MODE_PAGETABLE);
    switch(rc) {
        case USLOSS_MMU_OK: 
            result = P1_SUCCESS;
            break;
        case USLOSS_MMU_ERR_ON:
            result = P3_ALREADY_INITIALIZED;
            break;
        case USLOSS_MMU_ERR_PAGE:
            result = P3_INVALID_NUM_PAGES;
            break;
        case USLOSS_MMU_ERR_FRAME:
            result = P3_INVALID_NUM_FRAMES;
            break;
        default:
            USLOSS_Console("MMUInit: USLOSS_MmuInit returned %d\n", rc);
            USLOSS_Halt(1);
    }
    return result;
}

static int
MMUShutdown(void) 
{
    if (initialized) {
        int rc = USLOSS_MmuDone();
        assert(rc == USLOSS_MMU_OK);
    }
    return P1_SUCCESS;
}

static USLOSS_PTE *
PageTableAllocateIdentity(int pages)
{
    USLOSS_PTE  *table = NULL;
    // allocate and initialize table here
    if(!initialized)
        return NULL; //Returns NULL if P3_VmInit has not been called
    else{
        table = malloc(sizeof(USLOSS_PTE)*pages);
        for(int i = 0;i<pages;i++){
            table[i].frame=i; // identity, page X maps frame X
            table[i].read=l;
            table[i].write=l;
            table[i].incore=l;
        }
        numPages = pages;
        pageTables[P1_GetPid()]=table;
    }
    
    return table;
}

static int
PageTableFree(PID pid)
{
    int result = P1_SUCCESS;
    // free table here
    if(initialized){//Does nothing if P3_VmInit has not been called.
        free(pageTables[P1_GetPid()]);
        pageTables[P1_GetPid()]=NULL;
    }
    return result;
}

int P3_Startup(void *arg)
{
    int pid;
    int pid4;
    int status;
    int rc;

    rc = Sys_Spawn("P4_Startup", P4_Startup, NULL,  4 * USLOSS_MIN_STACK, 3, &pid4);
    assert(rc == 0);
    assert(pid4 >= 0);
    rc = Sys_Wait(&pid, &status);
    assert(rc == 0);
    assert(pid == pid4);
    Sys_VmShutdown();
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * P3_PrintStats --
 *
 *  Print out VM statistics.
 *
 * Results:
 *  None
 *
 * Side effects:
 *  Stuff is printed to the console.
 *
 *----------------------------------------------------------------------
 */
void
P3_PrintStats(P3_VmStats *stats)
{
    USLOSS_Console("P3_PrintStats:\n");
    USLOSS_Console("\tpages:\t\t%d\n", stats->pages);
    USLOSS_Console("\tframes:\t\t%d\n", stats->frames);
    USLOSS_Console("\tblocks:\t\t%d\n", stats->blocks);
    USLOSS_Console("\tfreeFrames:\t%d\n", stats->freeFrames);
    USLOSS_Console("\tfreeBlocks:\t%d\n", stats->freeBlocks);
    USLOSS_Console("\tfaults:\t\t%d\n", stats->faults);
    USLOSS_Console("\tnew:\t\t%d\n", stats->new);
    USLOSS_Console("\tpageIns:\t%d\n", stats->pageIns);
    USLOSS_Console("\tpageOuts:\t%d\n", stats->pageOuts);
    USLOSS_Console("\treplaced:\t%d\n", stats->replaced);
}

