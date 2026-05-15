/**
 * @file  lwip.c
 * @brief Bare-metal LwIP initialisation (static IP, NO_SYS = 1).
 */

#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "netif/ethernet.h"
#include "ethernetif.h"

struct netif gnetif;

void MX_LWIP_Init(void)
{
    ip4_addr_t ipaddr, netmask, gw;

    lwip_init();

    IP4_ADDR(&ipaddr,  192, 168, 1, 10);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw,      192, 168, 1, 1);

    netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL,
              &ethernetif_init, &ethernet_input);
    netif_set_default(&gnetif);
    netif_set_up(&gnetif);
}
