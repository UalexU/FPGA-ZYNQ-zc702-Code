#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "out.h"
#include "net.h"
#include "app_config.h"

#define OUT_LINE_MAX 192

void out_puts(const char *s)
{
    if (net_connected()) {
        net_send(s, (int)strlen(s));
        if (!OUT_UART_ALSO) {
            return;
        }
    }
    xil_printf("%s", s);
}

void out_printf(const char *fmt, ...)
{
    char buf[OUT_LINE_MAX];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);   /* truncates, always terminated */
    va_end(ap);

    out_puts(buf);
}
