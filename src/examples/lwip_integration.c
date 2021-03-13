// Debug messages
#define LWIP_DEBUG 1
#define NETIF_DEBUG LWIP_DBG_ON
#define TCP_DEBUG LWIP_DBG_ON
#define TCPIP_DEBUG LWIP_DBG_ON
#define UDP_DEBUG LWIP_DBG_ON

#include <hardware/gpio.h>

#include <pico/stdio.h>
#include <pico/util/queue.h>

#include <lwip/netif.h>
#include <lwip/init.h>
#include <lwip/timeouts.h>

#include <pico/enc28j60/enc28j60.h>
#include <pico/enc28j60/ethernetif.h>

#include "lwipopts.h"
#include "tcpecho_raw.h"

// Configuration
#define SPI spi0
#define SPI_BAUD 2000000
#define SCK_PIN 2
#define SI_PIN 3
#define SO_PIN 4
#define CS_PIN 10
#define INT_PIN 11
#define RX_QUEUE_SIZE 10
#define MAC_ADDRESS { 0x62, 0x5E, 0x22, 0x07, 0xDE, 0x92 }
#define IP_ADDRESS IPADDR4_INIT_BYTES(192, 168, 1, 200)
#define NETWORK_MASK IPADDR4_INIT_BYTES(255, 255, 255, 0)
#define GATEWAY_ADDRESS IPADDR4_INIT_BYTES(192, 168, 1, 1)

queue_t rx_queue;
critical_section_t enc28j60_cs;
enc28j60_t enc28j60 = {
        .spi = SPI,
        .cs_pin = CS_PIN,
        .mac_address = MAC_ADDRESS,
        .next_packet = 0,
        .critical_section = &enc28j60_cs,
};
struct netif netif;

void eth_irq(uint gpio, uint32_t events) {
    enc28j60_isr_begin(&enc28j60);
    uint8_t flags = enc28j60_interrupt_flags(&enc28j60);

    if (flags & ENC28J60_TXIF) {
        LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: transmit completed\n"));
    }

    if (flags & ENC28J60_PKTIF) {
        LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: packet pending\n"));
        struct pbuf *packet = low_level_input(&netif);
        if (packet != NULL) {
            if (!queue_try_add(&rx_queue, &packet)) {
                pbuf_free(packet);
            }
        }
    }

    if (flags & ENC28J60_TXERIE) {
        LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: transmit error\n"));
    }

    if (flags & (ENC28J60_TXIF | ENC28J60_TXERIF)) {
        uint8_t status[7];
        enc28j60_transfer_status(&enc28j60, status);
        if (ENC28J60_TX_STATUS_BIT(status, 51)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit VLAN Tagged Frame\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 50)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Backpressure Applied\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 49)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Pause Control Frame\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 48)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Control Frame\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 31)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Underrun\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 30)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Giant\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 29)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Late Collision\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 28)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Excessive Collision\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 27)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Excessive Defer\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 26)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Packet Defer\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 25)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Broadcast\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 24)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Multicast\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 23)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Done\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 22)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Length Out of Range\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 21)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit Length Check Error\n")); }
        if (ENC28J60_TX_STATUS_BIT(status, 20)) { LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: Transmit CRC Error\n")); }
    }

    if (flags & ENC28J60_RXERIE) {
        LWIP_DEBUGF(NETIF_DEBUG, ("eth_irq: receive error\n"));
    }

    enc28j60_interrupt_clear(&enc28j60, flags);
    enc28j60_isr_end(&enc28j60);
}

int main() {
    stdio_init_all();

    spi_init(spi0, SPI_BAUD);
    gpio_set_function(SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SO_PIN, GPIO_FUNC_SPI);
    gpio_init(CS_PIN);
    gpio_set_dir(CS_PIN, GPIO_OUT);

    queue_init(&rx_queue, sizeof(struct pbuf *), RX_QUEUE_SIZE);
    critical_section_init(&enc28j60_cs);

    lwip_init();
    const struct ip4_addr ipaddr = IP_ADDRESS;
    const struct ip4_addr netmask = NETWORK_MASK;
    const struct ip4_addr gw = GATEWAY_ADDRESS;
    netif_add(&netif, &ipaddr, &netmask, &gw, &enc28j60, ethernetif_init, netif_input);
    netif_set_up(&netif);
    netif_set_link_up(&netif);

    gpio_set_irq_enabled_with_callback(INT_PIN, GPIO_IRQ_EDGE_FALL, true, eth_irq);
    enc28j60_interrupts(&enc28j60, ENC28J60_PKTIE | ENC28J60_TXIE | ENC28J60_TXERIE | ENC28J60_RXERIE);

    tcpecho_raw_init();

    while (true) {
        struct pbuf* p = NULL;
        queue_try_remove(&rx_queue, &p);
        if (p != NULL) {
            if(netif.input(p, &netif) != ERR_OK) {
                LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
                pbuf_free(p);
            }
        }

        sys_check_timeouts();
    }
}
