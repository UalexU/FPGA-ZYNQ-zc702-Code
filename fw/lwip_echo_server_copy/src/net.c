/*
 * net.c -- lwIP TCP server for the SENSORS app.
 *
 * Needs, from the "lwIP Echo Server" template (keep these files):
 *   platform.c, platform_zynq.c, platform.h, platform_config.h(.in)
 * and the lwIP library in the BSP (the template adds it).
 *
 * lwIP timers: the template drives tcp_fasttmr/tcp_slowtmr from the BSP's
 * xiltimer "tick timer". If the BSP has no tick timer configured
 * (XTIMER_NO_TICK_TIMER, which is the case on this platform), that
 * interrupt never fires, so net_poll() runs the timers itself from the
 * global timer. With a tick timer configured, the template flags are used.
 */

#include <stddef.h>

#include "net.h"
#include "app_config.h"

#include "xparameters.h"
#include "netif/xadapter.h"
#include "lwip/init.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "platform_config.h"    /* PLATFORM_EMAC_BASEADDR */
#include "ticks.h"

/* From the template's platform_zynq.c. Declared here instead of including
 * platform.h, so nothing in this app depends on that header. */
void platform_enable_interrupts(void);

#if LWIP_IPV6 == 1
#  error "This code is IPv4: set lwIP ipv6_enable = false in the BSP"
#endif

#if defined(XTIMER_NO_TICK_TIMER)
#  define NET_SOFT_TIMERS 1             /* no tick interrupt: time it here */
#else
#  define NET_SOFT_TIMERS 0             /* platform tick sets the flags */
#endif

/* Set by the platform timer interrupt (platform.c / platform_zynq.c). */
extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;
void tcp_fasttmr(void);
void tcp_slowtmr(void);

static struct netif    board_netif;

/* The template's platform timer code refers to this name. */
struct netif *echo_netif = &board_netif;

#if NET_SOFT_TIMERS
static Ticks t_fast, t_slow, t_link;
#endif
static struct tcp_pcb *client;          /* NULL = nobody connected */
static int             net_up;
static int             client_is_new;

/* Received bytes wait here until the main loop reads them. */
#define RX_SIZE 256
static u8  rx_buf[RX_SIZE];
static u16 rx_head, rx_tail;

static void rx_put(u8 c)
{
    u16 next = (u16)((rx_head + 1) % RX_SIZE);

    if (next != rx_tail) {              /* full: drop the byte */
        rx_buf[rx_head] = c;
        rx_head = next;
    }
}

/* ---------------- lwIP callbacks ---------------- */

/* Connection died (reset, timeout). lwIP already freed the pcb. */
static void err_cb(void *arg, err_t err)
{
    (void)arg;
    (void)err;
    client = NULL;
}

static err_t recv_cb(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    struct pbuf *q;
    u16 i;

    (void)arg;

    if (p == NULL) {                    /* PC closed the connection */
        if (pcb == client) {
            client = NULL;
        }
        tcp_arg(pcb, NULL);
        tcp_recv(pcb, NULL);
        tcp_err(pcb, NULL);
        if (tcp_close(pcb) != ERR_OK) {
            tcp_abort(pcb);
            return ERR_ABRT;
        }
        return ERR_OK;
    }
    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    for (q = p; q != NULL; q = q->next) {
        const u8 *d = (const u8 *)q->payload;
        for (i = 0; i < q->len; i++) {
            rx_put(d[i]);
        }
    }
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static err_t accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    (void)arg;

    if (err != ERR_OK || newpcb == NULL) {
        return ERR_VAL;
    }

    /* Newest connection wins, so a PC that vanished without closing
     * (unplugged cable, crashed script) can't lock everyone out. */
    if (client != NULL) {
        struct tcp_pcb *old = client;
        client = NULL;
        tcp_arg(old, NULL);
        tcp_recv(old, NULL);
        tcp_err(old, NULL);
        tcp_abort(old);
    }

    client = newpcb;
    tcp_nagle_disable(newpcb);          /* send each line right away */
    tcp_recv(newpcb, recv_cb);
    tcp_err(newpcb, err_cb);

    rx_head = rx_tail = 0;
    client_is_new = 1;
    return ERR_OK;
}

/* ---------------- public ---------------- */

int net_init(void)
{
    ip_addr_t ip, mask, gw;
    unsigned char mac[6] = NET_MAC;
    struct tcp_pcb *pcb;

    ipaddr_aton(NET_IP, &ip);
    ipaddr_aton(NET_MASK, &mask);
    ipaddr_aton(NET_GW, &gw);

    lwip_init();

    if (!xemac_add(&board_netif, &ip, &mask, &gw, mac,
                   PLATFORM_EMAC_BASEADDR)) {
        xil_printf("# NET: xemac_add failed\r\n");
        return -1;
    }
    netif_set_default(&board_netif);

#ifndef SDT
    platform_enable_interrupts();
#endif

    netif_set_up(&board_netif);

    pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (pcb == NULL || tcp_bind(pcb, IP_ANY_TYPE, NET_PORT) != ERR_OK) {
        xil_printf("# NET: cannot bind port %d\r\n", NET_PORT);
        return -1;
    }
    pcb = tcp_listen(pcb);
    if (pcb == NULL) {
        xil_printf("# NET: tcp_listen out of memory\r\n");
        return -1;
    }
    tcp_accept(pcb, accept_cb);

#if NET_SOFT_TIMERS
    t_fast = t_slow = t_link = ticks_now();
#endif
    net_up = 1;
    xil_printf("# NET: listening on %s:%d\r\n", NET_IP, NET_PORT);
    return 0;
}

void net_poll(void)
{
    if (!net_up) {
        return;
    }
#if NET_SOFT_TIMERS
    {
        Ticks now = ticks_now();

        if (now - t_fast >= TICKS_FROM_US(250000)) {   /* lwIP: 250 ms */
            t_fast = now;
            tcp_fasttmr();
        }
        if (now - t_slow >= TICKS_FROM_US(500000)) {   /* lwIP: 500 ms */
            t_slow = now;
            tcp_slowtmr();
        }
        if (now - t_link >= TICKS_FROM_US(1000000)) {  /* PHY link check */
            t_link = now;
            eth_link_detect(&board_netif);   /* re-negotiates after replug */
        }
    }
#else
    if (TcpFastTmrFlag) {
        tcp_fasttmr();
        TcpFastTmrFlag = 0;
    }
    if (TcpSlowTmrFlag) {
        tcp_slowtmr();
        TcpSlowTmrFlag = 0;
    }
#endif
    xemacif_input(&board_netif);        /* may call the callbacks above */
}

int net_connected(void)
{
    return client != NULL;
}

int net_new_client(void)
{
    int was_new = client_is_new && client != NULL;

    client_is_new = 0;
    return was_new;
}

int net_getchar(void)
{
    int c;

    if (rx_head == rx_tail) {
        return -1;
    }
    c = rx_buf[rx_tail];
    rx_tail = (u16)((rx_tail + 1) % RX_SIZE);
    return c;
}

/* Never blocks: if the PC isn't keeping up, the line is dropped. */
int net_send(const char *buf, int len)
{
    if (client == NULL || len <= 0) {
        return -1;
    }
    if (tcp_sndbuf(client) < len) {
        return -1;
    }
    if (tcp_write(client, buf, (u16)len, TCP_WRITE_FLAG_COPY) != ERR_OK) {
        return -1;
    }
    tcp_output(client);
    return 0;
}
