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
    if(USLOSS_MmuGetCause()==USLOSS_MMU_FAULT){
        int pid = P1_GetPid();
        USLOSS_PTE *table;
        int rc = P3PageTableGet(pid,&table);
        if(rc!=P1_SUCCESS||table==NULL){
            USLOSS_Console("PAGE FAULT!!! PID %d offset 0x%x\n", P1_GetPid(), (int) arg);
            USLOSS_Halt(1);
        }else{
        	// determine which page suffered the fault, not sure about this step
            int page = ((int) arg)/USLOSS_MmuPageSize();
            (table+page)->incore=1;
            (table+page)->read=1;
            (table+page)->write=1;
            (table+page)->frame=page;
            rc = USLOSS_MmuSetPageTable(table);
        }
    }else{
        USLOSS_Console("PAGE FAULT!!! PID %d offset 0x%x\n", P1_GetPid(), (int) arg);
        USLOSS_Halt(1);
    }

}

USLOSS_PTE *
P3PageTableAllocateEmpty(int pages)
{
    USLOSS_PTE  *table = NULL;
    table=malloc(sizeof(USLOSS_PTE)*pages);
    for(int i=0;i<pages;i++){
        (table+i)->incore=0;
    }
    return table;
}
