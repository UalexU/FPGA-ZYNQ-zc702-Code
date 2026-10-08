/*
 * app_config.h -- every build option in one place.
 * SENSORS app, Trenz TE0745 (Zynq XC7Z045) on TEB0745, with Ethernet.
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "xil_printf.h"

/* ===================== build options ===================== */

/* 1 = checkpoints, register dumps and raw frames on the UART (UART only). */
#define DEBUG_MODE          0

/* 1 = only the MAX31865, on axi_quad_spi_0 in mode 3 (no TMAG5170).
 * 0 = TMAG5170 on axi_quad_spi_0, MAX31865 per ENABLE_RTD. */
#define RTD_ONLY            1

/* Only used when RTD_ONLY is 0: 1 = MAX31865 on axi_quad_spi_1 (mode 3). */
#define ENABLE_RTD          0

/* Printed in the five TMAG columns when RTD_ONLY is 1. */
#define TMAG_PLACEHOLDER    "nan"

/* 1 = probe axi_iic_0 at boot and print what ACKs. */
#define ENABLE_I2C_SCAN     0

/* 1 = CSV for the GUI.  0 = labelled text for a human. */
#define OUTPUT_CSV          1

/* Boot rate. Runtime-settable afterwards with 'R'. */
#define SAMPLE_PERIOD_US    330000
#define MIN_RATE_HZ         1
#define MAX_RATE_HZ         5000

/* Reported to the host only (50 MHz FCLK_CLK0 / 16). */
#define SPI_SCK_KHZ         3125

/* ===================== network ===================== */

/* Static IP: direct cable to the PC, no DHCP server. PC = 192.168.1.100. */
#define NET_IP              "192.168.1.10"
#define NET_MASK            "255.255.255.0"
#define NET_GW              "192.168.1.1"
#define NET_PORT            7
#define NET_MAC             { 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 }

/* While a TCP client is connected, output goes to TCP only.
 * 1 = also copy it to the UART (limits the rate to what 115200 baud can carry). */
#define OUT_UART_ALSO       0

/* ===================== TMAG5170 boot settings ===================== */

/*   0x1 = +/-25 mT (2500)   0x0 = +/-50 mT (5000)   0x2 = +/-100 mT (10000) */
#define RANGE_CODE          0x2
#define RANGE_MT_X100       10000
#define CONV_AVG            0x5     /* 0h=1x .. 5h=32x */

/* ===================== MAX31865 board ===================== */

#define RTD_WIRE_MODE       0x00    /* 0x00 = 2/4-wire, 0x10 = 3-wire */
#define RTD_FILTER          0x00    /* 0x00 = 60 Hz notch, 0x01 = 50 Hz */
#define RTD_R0_MOHM         100000  /* PT100 */
#define RTD_RREF_MOHM       400000  /* 430 ohm: READ THE RESISTOR ON YOUR BOARD */
#define RTD_LEAD_MOHM       0       /* 2-wire lead resistance, both leads */
#define RTD_ALPHA_PPM       3850

/* ===================== derived -- don't edit ===================== */

#if RTD_ONLY
#  define HAVE_TMAG     0
#  define HAVE_RTD      1
#else
#  define HAVE_TMAG     1
#  define HAVE_RTD      ENABLE_RTD
#endif

#if HAVE_RTD
#  define RTD_CONV_HZ   ((RTD_FILTER & 0x01) ? 50 : 60)
#else
#  define RTD_CONV_HZ   0       /* reported as rtd_hz=0: not fitted */
#endif

#if DEBUG_MODE
#  define DBG(...)  xil_printf(__VA_ARGS__)
#else
#  define DBG(...)  ((void)0)
#endif

#endif /* APP_CONFIG_H */
