/*
 * sensors.h -- TMAG5170 + MAX31865 over AXI Quad SPI.
 * Which devices exist is set in app_config.h (RTD_ONLY, ENABLE_RTD).
 */
#ifndef SENSORS_H
#define SENSORS_H

#include "xil_types.h"

typedef struct {
    s32 bx, by, bz;     /* mT x100 */
    s32 mag;            /* |B|, mT x100 */
    s32 dieC;           /* TMAG die temperature, C x100 */
    s32 rtdC;           /* RTD temperature, C x100 */
    s32 rtd_mohm;       /* RTD resistance, milliohm */
    int rtd_fault;      /* 1 = rtdC is not valid */
} SensorSample;

int  sensors_init(void);                 /* 0 = ok, -1 = SPI setup failed */
void sensors_read(SensorSample *s);

/* Runtime TMAG settings.
 * Return 0 = ok, -1 = bad value, -2 = readback mismatch, -3 = no TMAG. */
int  sensors_set_avg(int mult);          /* 1 2 4 8 16 32 */
int  sensors_set_range(int mt);          /* 25 50 100 */
int  sensors_avg_mult(void);
int  sensors_range_mt(void);

#endif /* SENSORS_H */
