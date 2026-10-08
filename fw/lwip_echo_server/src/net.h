/*
 * net.h -- one TCP client over lwIP (raw API), static IPv4.
 *
 * The board listens on NET_IP:NET_PORT. One PC connects; a newer
 * connection replaces an older one. Everything is polled from the main
 * loop -- no callbacks reach the application.
 */
#ifndef NET_H
#define NET_H

int  net_init(void);                       /* 0 = ok */
void net_poll(void);                       /* call as often as possible */

int  net_connected(void);                  /* 1 while a client is connected */
int  net_new_client(void);                 /* 1 once, right after a connect */

int  net_getchar(void);                    /* next received byte, -1 if none */
int  net_send(const char *buf, int len);   /* 0 = queued, -1 = dropped */

#endif /* NET_H */
