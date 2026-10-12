/*
 * out.h -- all host-facing text goes through here.
 * TCP client connected -> sent over Ethernet (and UART if OUT_UART_ALSO).
 * No client           -> UART, exactly as before.
 */
#ifndef OUT_H
#define OUT_H

void out_puts(const char *s);
void out_printf(const char *fmt, ...);

#endif /* OUT_H */
