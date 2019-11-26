/*
 * test_out_of_swap.c
 *  
 *  Tests that the Child is killed when the swap runs out.
 *
 */
#include <usyscall.h>
#include <libuser.h>
#include <assert.h>
#include <usloss.h>
#include <stdlib.h>
#include <phase3.h>
#include <stdarg.h>
#include <unistd.h>

#include "tester.h"
#include "phase3Int.h"

#define PAGES 1             // # of pages
#define FRAMES 1            // # of frames
#define PAGERS 2            // # of pagers

static char *vmRegion;
static int  pageSize;

static int passed = FALSE;

#ifdef DEBUG
int debugging = 1;
#else
int debugging = 0;
#endif /* DEBUG */

static void
Debug(char *fmt, ...)
{
    va_list ap;

    if (debugging) {
        va_start(ap, fmt);
        USLOSS_VConsole(fmt, ap);
    }
}
static int
Child(void *arg)
{
    int     pid;
    char    dummy;

    Sys_GetPID(&pid);
    Debug("Child (%d) starting.\n", pid);

    // Read the first page. This should cause an out-of-swap error and we should die.
    dummy = *vmRegion;
    // Should not get here.
    passed = FALSE;
    Debug("Child still alive!!\n");
    return 1;
}

static PID childPID;

int
P4_Startup(void *arg)
{
    int     rc;
    int     pid;
    int     status;

    Debug("P4_Startup starting.\n");
    rc = Sys_VmInit(PAGES, PAGES, FRAMES, PAGERS, (void **) &vmRegion);
    TEST(rc, P1_SUCCESS);

    pageSize = USLOSS_MmuPageSize();
    rc = Sys_Spawn("Child", Child, NULL, USLOSS_MIN_STACK * 4, 3, &childPID);
    assert(rc == P1_SUCCESS);
    rc = Sys_Wait(&pid, &status);
    assert(rc == P1_SUCCESS);
    TEST(status, P3_OUT_OF_SWAP);
    Debug("Child terminated\n");
    Sys_VmShutdown();
    PASSED();
    return 0;
}


void test_setup(int argc, char **argv) {
}

void test_cleanup(int argc, char **argv) {
    if (passed) {
        USLOSS_Console("TEST PASSED.\n");
    }
}

// Phase 3d stubs

#include "phase3Int.h"

int P3SwapInit(int pages, int frames) {return P1_SUCCESS;}
int P3SwapShutdown(void) {return P1_SUCCESS;}
int P3SwapFreeAll(PID pid) {return P1_SUCCESS;}
int P3SwapOut(int *frame) {return P1_SUCCESS;}
int P3SwapIn(PID pid, int page, int frame) {return P3_OUT_OF_SWAP;}



