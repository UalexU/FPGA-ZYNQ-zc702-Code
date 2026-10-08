/*
 * main.c -- SENSORS app, Trenz TE0745 (Zynq XC7Z045) on TEB0745,
 * streaming over Ethernet (TCP) with the PS UART as fallback.
 *
 *   app_config.h   all build options (sensors, network)
 *   sensors.c/.h   TMAG5170 + MAX31865 over AXI Quad SPI
 *   net.c/.h       lwIP TCP server, one client, static IP
 *   out.c/.h       text output: TCP when a client is connected, else UART
 *   main.c         command parser + timed main loop (this file)
 *
 * Connect from the PC to 192.168.1.10 port 7. On connect the board sends
 * the CONFIG line and the CSV header, then one CSV line per sample.
 * Commands work identically over TCP or the UART, one per line:
 *
 *   R <hz>    main-loop rate, 1..5000 Hz
 *   A <mult>  TMAG averaging, 1 2 4 8 16 32     (refused when RTD_ONLY)
 *   G <mt>    magnetic range, 25 50 100         (refused when RTD_ONLY)
 *   P <0|1>   pause / resume streaming
 *   Z         re-send the CONFIG line
 *
 * The loop never sleeps: lwIP must be polled continuously, so samples are
 * timed with the global timer (ticks.h) instead of usleep().
 */

#include <stdio.h>

#include "app_config.h"
#include "sensors.h"
#include "net.h"
#include "out.h"
#include "ticks.h"

#include "xparameters.h"
#include "xil_io.h"
#include <bspconfig.h>

/* From the template's platform.c. Declared here rather than including
 * platform.h, which uses lwIP types (u32_t) and needs lwIP headers first. */
void init_platform(void);

/* ===================== UART command channel ===================== */

#if defined(STDIN_BASEADDRESS)
#  define UART_BASE         STDIN_BASEADDRESS
#elif defined(XPAR_XUARTPS_0_BASEADDR)
#  define UART_BASE         XPAR_XUARTPS_0_BASEADDR
#else
#  error "No PS UART found: enable UART0/1 in the Zynq PS and set it as stdin/stdout"
#endif
#define UART_STATUS         0x2C
#define UART_RX_FIFO        0x30
#define UART_SR_RX_EMPTY    0x02

#define CMD_MAX             24

typedef struct {
    char buf[CMD_MAX];
    int  len;
} LineBuf;

static LineBuf uart_line, net_line;

/* ===================== runtime state ===================== */

static u32 sample_period_us = SAMPLE_PERIOD_US;
static int streaming        = 1;

/* ===================== output ===================== */

static void report_config(void)
{
    out_printf("# CONFIG conv_avg=%d range_mt=%d axes=3 temp=1 rtd_hz=%d "
               "period_us=%d sck_khz=%d tmag=%d\r\n",
               sensors_avg_mult(), sensors_range_mt(), RTD_CONV_HZ,
               (int)sample_period_us, SPI_SCK_KHZ, HAVE_TMAG);
}

static void print_header(void)
{
#if OUTPUT_CSV
    out_puts("Bx,By,Bz,Bmag,DieC,RtdC\r\n");
#else
    out_puts("\r\nSENSORS on TE0745 (Zynq)\r\n");
#endif
}

/* Value x100 as "12.34". Sign handled separately: -50/100 is 0 in C. */
static int fmt_x100(char *dst, s32 v)
{
    s32 whole = v / 100;
    s32 frac  = v % 100;

    if (frac < 0) {
        frac = -frac;
    }
    if (v < 0 && whole == 0) {
        return sprintf(dst, "-0.%02d", (int)frac);
    }
    return sprintf(dst, "%d.%02d", (int)whole, (int)frac);
}

/* One sample = one line = one TCP write. */
static void send_sample(const SensorSample *s)
{
    char line[160];
    char *p = line;

#if OUTPUT_CSV
    if (HAVE_TMAG) {
        p += fmt_x100(p, s->bx);   p += sprintf(p, ", ");
        p += fmt_x100(p, s->by);   p += sprintf(p, ", ");
        p += fmt_x100(p, s->bz);   p += sprintf(p, ", ");
        p += fmt_x100(p, s->mag);  p += sprintf(p, ", ");
        p += fmt_x100(p, s->dieC); p += sprintf(p, ", ");
    } else {
        p += sprintf(p, TMAG_PLACEHOLDER ", " TMAG_PLACEHOLDER ", "
                        TMAG_PLACEHOLDER ", " TMAG_PLACEHOLDER ", "
                        TMAG_PLACEHOLDER ", ");
    }
    if (HAVE_RTD && !s->rtd_fault) {
        p += fmt_x100(p, s->rtdC);
    } else {
        p += sprintf(p, "nan");
    }
    sprintf(p, "\r\n");
#else
    if (HAVE_TMAG) {
        p += sprintf(p, "B ");
        p += fmt_x100(p, s->bx);   p += sprintf(p, " ");
        p += fmt_x100(p, s->by);   p += sprintf(p, " ");
        p += fmt_x100(p, s->bz);   p += sprintf(p, " mT | |B| ");
        p += fmt_x100(p, s->mag);  p += sprintf(p, " mT | die ");
        p += fmt_x100(p, s->dieC); p += sprintf(p, " C | ");
    }
    if (HAVE_RTD) {
        s32 r = s->rtd_mohm;
        p += sprintf(p, "rtd ");
        p += fmt_x100(p, s->rtdC);
        sprintf(p, " C  R=%d.%03d ohm%s\r\n", (int)(r / 1000),
                (int)((r < 0 ? -r : r) % 1000), s->rtd_fault ? "  FAULT" : "");
    } else {
        sprintf(p, "rtd n/a\r\n");
    }
#endif

    out_puts(line);
}

/* ===================== commands ===================== */

static int uart_getchar(void)
{
    if (Xil_In32(UART_BASE + UART_STATUS) & UART_SR_RX_EMPTY) {
        return -1;
    }
    return (int)(Xil_In32(UART_BASE + UART_RX_FIFO) & 0xFF);
}

/* Decimal integer after the command letter, or -1 if there isn't one. */
static int parse_int(const char *s)
{
    int value = 0, digits = 0;

    while (*s == ' ' || *s == '\t') {
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
        digits++;
        if (value > 1000000) {
            return -1;
        }
    }
    return digits ? value : -1;
}

static void handle_command(char *line)
{
    int arg = parse_int(line + 1);
    int rc;

    switch (line[0]) {

    case 'R':
        if (arg < MIN_RATE_HZ || arg > MAX_RATE_HZ) {
            out_printf("# ERR R: want %d..%d Hz\r\n", MIN_RATE_HZ, MAX_RATE_HZ);
            return;
        }
        sample_period_us = (u32)(1000000 / arg);
        break;

    case 'A':
        rc = sensors_set_avg(arg);
        if (rc == -3) { out_puts("# ERR A: no TMAG5170 in this build (RTD_ONLY)\r\n"); return; }
        if (rc == -1) { out_puts("# ERR A: want 1 2 4 8 16 or 32\r\n");                return; }
        if (rc == -2) { out_puts("# ERR A: DEVICE_CONFIG did not take\r\n");           return; }
        break;

    case 'G':
        rc = sensors_set_range(arg);
        if (rc == -3) { out_puts("# ERR G: no TMAG5170 in this build (RTD_ONLY)\r\n"); return; }
        if (rc == -1) { out_puts("# ERR G: want 25, 50 or 100 mT\r\n");               return; }
        if (rc == -2) { out_puts("# ERR G: SENSOR_CONFIG did not take\r\n");           return; }
        break;

    case 'P':
        if (arg != 0 && arg != 1) {
            out_puts("# ERR P: want 0 or 1\r\n");
            return;
        }
        streaming = arg;
        break;

    case 'Z':
        break;

    default:
        out_printf("# ERR unknown command '%c'\r\n", line[0]);
        return;
    }

    out_printf("# ACK %s\r\n", line);
    report_config();
}

/* Collect characters into a line; act on complete lines only. */
static void feed_char(LineBuf *lb, int c)
{
    if (c == '\r' || c == '\n') {
        if (lb->len > 0) {
            lb->buf[lb->len] = '\0';
            handle_command(lb->buf);
            lb->len = 0;
        }
    } else if (lb->len < CMD_MAX - 1) {
        lb->buf[lb->len++] = (char)c;
    } else {
        lb->len = 0;                    /* overlong line: drop it */
    }
}

static void poll_commands(void)
{
    int c;

    while ((c = uart_getchar()) >= 0) {
        feed_char(&uart_line, c);
    }
    while ((c = net_getchar()) >= 0) {
        feed_char(&net_line, c);
    }
}

/* ===================== timing ===================== */

static Ticks period_ticks(void)
{
    return TICKS_FROM_US(sample_period_us);
}

/* ===================== main ===================== */

int main(void)
{
    SensorSample s;
    Ticks now, next_due;

    init_platform();                    /* caches, timer + interrupts for lwIP */

    if (sensors_init() != 0) {
        xil_printf("# sensor init failed -- halting\r\n");
        return -1;
    }

    if (net_init() != 0) {
        xil_printf("# Ethernet init failed -- UART only\r\n");
    }

    report_config();
    print_header();

    next_due = ticks_now();

    while (1) {
        net_poll();

        if (net_new_client()) {         /* PC just connected: tell it the setup */
            net_line.len = 0;
            report_config();
            print_header();
        }

        poll_commands();

        if (!streaming) {
            continue;
        }

        now = ticks_now();
        if ((s64)(now - next_due) < 0) {
            continue;                   /* not time yet: keep polling */
        }
        next_due += period_ticks();
        if ((s64)(now - next_due) > 0) {
            next_due = now + period_ticks();   /* fell behind: don't burst */
        }

        sensors_read(&s);
        send_sample(&s);
    }

    return 0;
}
