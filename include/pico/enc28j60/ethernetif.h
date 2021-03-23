#ifndef ENC28J60_ETHERNETIF_H
#define ENC28J60_ETHERNETIF_H

err_t ethernetif_init(struct netif *netif);
struct pbuf *low_level_input(const struct netif *netif);

#endif
