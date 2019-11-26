/*
 * p3stubs.c
 *
 * These are stub implementations of functions defined later parts of Phase 3.
 */

#include <stdio.h>
#include <string.h>
#include <usloss.h>
#include <assert.h>
#include <phase1.h>

#include "phase3Int.h"

// Phase 3c

int P3FrameInit(int pages, int frames) {return P1_SUCCESS;}
int P3FrameShutdown(void) {return P1_SUCCESS;}
int P3FrameAllocate(PID pid, int *frame) {return P1_SUCCESS;}
int P3FrameFreeAll(PID pid) {return P1_SUCCESS;}
int P3PagerInit(int pages, int frames, int pagers) {return P1_SUCCESS;}
int P3PagerShutdown(void) {return P1_SUCCESS;}

// Phase 3d

int P3SwapInit(int pages, int frames) {return P1_SUCCESS;}
int P3SwapShutdown(void) {return P1_SUCCESS;}
int P3SwapFreeAll(PID pid) {return P1_SUCCESS;}
int P3SwapClock(PID pid, int *frame) {return P1_SUCCESS;}
int P3SwapIn(PID pid, int page, int frame) {return P1_SUCCESS;}
