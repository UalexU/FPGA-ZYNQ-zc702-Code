/*
 * ticks.h -- free-running time base (Cortex-A9 global timer, CPU clock / 2).
 * Same counter the BSP's usleep() uses, read through the BSP's own API.
 */
#ifndef TICKS_H
#define TICKS_H

#include "xil_types.h"
#ifdef SDT
#  include "xiltimer.h"     /* Vitis 2023.2+: XTime, XTime_GetTime, COUNTS_PER_SECOND */
#else
#  include "xtime_l.h"      /* classic BSP */
#endif

typedef XTime Ticks;

#define TICKS_PER_SEC       ((u64)(COUNTS_PER_SECOND))
#define TICKS_FROM_US(us)   ((Ticks)((u64)(us) * TICKS_PER_SEC / 1000000u))

static inline Ticks ticks_now(void)
{
    XTime t;

    XTime_GetTime(&t);
    return (Ticks)t;
}

#endif /* TICKS_H */
