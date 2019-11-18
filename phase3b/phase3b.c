/*
 * phase3b.c
 *
 */

#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <usloss.h>
#include <string.h>
#include <libuser.h>

#include "phase3Int.h"

void
P3PageFaultHandler(int type, void *arg)
{
    /*******************

    if the cause is USLOSS_MMU_FAULT (USLOSS_MmuGetCause)
        if the process does not have a page table  (P3PageTableGet)
            print error message
            USLOSS_Halt(1)
        else
            determine which page suffered the fault (USLOSS_MmuPageSize)
            update the page's PTE to map page x to frame x
            set the PTE to be read-write and incore
            update the page table in the MMU (USLOSS_MmuSetPageTable)
    else
        print error message
        USLOSS_Halt(1)
    *********************/

}

USLOSS_PTE *
P3PageTableAllocateEmpty(int pages)
{
    USLOSS_PTE  *table = NULL;
    // allocate and initialize an empty page table
    return table;
}
