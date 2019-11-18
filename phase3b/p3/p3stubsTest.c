// Tests for p3stubs.c

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include "phase1.h"

extern int p3aborts;
extern int p3mode;

#define CheckAborts(diff) \
    if (p3aborts - current != (diff)) { \
        printf("Test %d failed.\n", count); \
        exit(1); \
    }

int main(int argc, char **argv)
{

    USLOSS_PTE *table;
    int     count = 1;
    int     current = 0;

    p3mode = 0; // Do not abort but count them instead.

    // Normal allocation

    current = p3aborts;
    table = (USLOSS_PTE *) 42;
    table = P3_AllocatePageTable(0);
    assert(table == NULL);
    CheckAborts(0);
    count++;

    // Normal free.

    current = p3aborts;
    P3_FreePageTable(0);
    CheckAborts(0);
    count++;

    // Invalid pid tests.
    current = p3aborts;
    table = P3_AllocatePageTable(-1);
    CheckAborts(1);
    count++;

    current = p3aborts;
    table = P3_AllocatePageTable(P1_MAXPROC);
    CheckAborts(1);
    count++;

    current = p3aborts;
    P3_FreePageTable(-1);
    CheckAborts(1);
    count++;

    current = p3aborts;
    P3_FreePageTable(P1_MAXPROC);
    CheckAborts(1);
    count++;

    // Duplicate allocate
    current = p3aborts;
    table = P3_AllocatePageTable(1);
    table = P3_AllocatePageTable(1);
    P3_FreePageTable(1);
    CheckAborts(1);
    count++;

    // Duplicate free
    current = p3aborts;
    table = P3_AllocatePageTable(0);
    P3_FreePageTable(0);
    P3_FreePageTable(0);
    CheckAborts(1);
    count++;

    // Free w/out allocate

    current = p3aborts;
    P3_FreePageTable(2);
    CheckAborts(1);
    count++;

    printf("All tests passed.\n");
}






 
