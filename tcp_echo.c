#include <memory.h>
#include <pico/util/queue.h>
#include <pico/critical_section.h>
#include <lwip/netif.h>
#include <lwip/snmp.h>
#include <lwip/etharp.h>
#include <pico/stdio.h>
#include <hardware/gpio.h>
#include <lwip/init.h>
#include <mdns_out.h>
#include <http_client.h>
#include <pico/enc28j60.h>

#include "lwipopts.h"


spi_inst_t * const spi = spi0;  // SPI bus
const uint8_t sck_pin = 2;  // SPI clock
const uint8_t mosi_pin = 3;  // SPI slave input
const uint8_t miso_pin = 4;  // SPI slave output
const uint8_t cs_pin = 10;  // SPI slave select
const uint8_t int_pin = 11;  // interrupt
const uint8_t mac_address[ETH_HWADDR_LEN] = {0x62, 0x5e, 0x22, 0x07, 0xde, 0x92};
const size_t input_q_size = 10;
const ip4_addr_t ip_address[4] = IPADDR4_INIT_BYTES(192, 168, 1, 200);
const ip4_addr_t netmask[4] = IPADDR4_INIT_BYTES(255, 255, 255, 0);
const ip4_addr_t gateway[4] = IPADDR4_INIT_BYTES(192, 168, 1, 1);
const uint16_t listen_port = 4242;

uint8_t packet_buffer[1500];  // Incoming packet buffer
uint16_t next_packet = 0;  // Address of next packet (ENC28J60 buffer memory)

enc28j60_config_t config;  // ENC28J60 configuration
queue_t input_q;  // Incoming packet queue
critical_section_t tx_cs;  // Transmit critical section
struct netif netif;  // Network interface configuration

err_t output_netif(struct netif *netif, struct pbuf *p) {
    LINK_STATS_INC(link.xmit);
    critical_section_enter_blocking(&tx_cs);
    pbuf_copy_partial(p, packet_buffer, p->tot_len, 0);
    enc28j60_transmit_packet(&config, packet_buffer, p->tot_len);
    critical_section_exit(&tx_cs);

    return ERR_OK;
}

err_t init_netif(struct netif *netif) {
    netif->hwaddr_len = ETH_HWADDR_LEN;
    memcpy(netif->hwaddr, mac_address, ETH_HWADDR_LEN);
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->output = etharp_output;
    netif->linkoutput = output_netif;

    return ERR_OK;
}

err_t recv_tcp(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    critical_section_enter_blocking(&tx_cs);
    pbuf_copy_partial(p, packet_buffer, p->tot_len, 0);
    tcp_write(tpcb, packet_buffer, p->tot_len, TCP_WRITE_FLAG_COPY);
    critical_section_exit(&tx_cs);
    tcp_recved(tpcb, p->tot_len);

    return ERR_OK;
}

err_t accept_tcp(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, recv_tcp);
    return ERR_OK;
}

void rx_irq(uint gpio, uint32_t events) {
    enc28j60_bit_clear(&config, ENC28J60_EIE, ENC28J60_INTIE);

    uint8_t interrupt_flags = enc28j60_read_cr8(&config, ENC28J60_EIR, false);

    if (interrupt_flags & ENC28J60_LINKIF) {
        printf("Link change interrupt\n");
        enc28j60_bit_clear(&config, ENC28J60_EIR, ENC28J60_LINKIF);
    }

    if (interrupt_flags & ENC28J60_TXIF) {
        printf("Transmit\n");
        enc28j60_bit_clear(&config, ENC28J60_EIR, ENC28J60_TXIF);
    }

    if (interrupt_flags & ENC28J60_TXERIF) {
        printf("Transmit error\n");
        enc28j60_bit_clear(&config, ENC28J60_EIR, ENC28J60_TXERIF);
    }

    if (interrupt_flags & ENC28J60_RXERIF) {
        printf("Receive error\n");
        enc28j60_bit_clear(&config, ENC28J60_EIR, ENC28J60_RXERIF);
    }

    critical_section_enter_blocking(&tx_cs);
    enc28j60_switch_bank(&config, 1);
    uint8_t packet_count = enc28j60_read_cr8(&config, ENC28J60_EPKTCNT, false);
    if (packet_count) {
        printf("Receive packet pending\n");
        size_t packet_size = enc28j60_receive_packet(&config, &next_packet, packet_buffer);
        struct pbuf *p = pbuf_alloc(PBUF_RAW, packet_size, PBUF_POOL);
        if (p != NULL) {
            pbuf_take(p, packet_buffer, packet_size);
            if (!queue_try_add(&input_q, &p)) {
                pbuf_free(p);
            }
        }
        enc28j60_bit_clear(&config, ENC28J60_EIR, ENC28J60_PKTIF);
    }
    critical_section_exit(&tx_cs);

    enc28j60_bit_set(&config, ENC28J60_EIE, ENC28J60_INTIE);
}

int main() {
    stdio_init_all();

    // SPI setup
    spi_init(spi0, 2000000);
    gpio_set_function(sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(miso_pin, GPIO_FUNC_SPI);
    gpio_init(cs_pin);
    gpio_set_dir(cs_pin, GPIO_OUT);

    // Initialize config, queue and critical section
    config = enc28j60_get_default_config();
    config.spi = spi;
    config.cs_pin = cs_pin;
    memcpy(config.mac_address, mac_address, ETH_HWADDR_LEN);
    queue_init(&input_q, sizeof (struct pbuf *), input_q_size);
    critical_section_init(&tx_cs);

    // Initialize ENC28J60
    enc28j60_reset(&config);
    enc28j60_init(&config);

    // Initialize network interface
    lwip_init();
    netif_add(&netif, ip_address, netmask, gateway, NULL, init_netif, netif_input);
    assert(netif_is_up(&netif));
    assert(netif_is_link_up(&netif));
    assert(!ip4_addr_isany_val(*netif_ip4_addr(&netif)));
    assert(ip4_route(gateway) == &netif);

    // ENC28J60 setup
    gpio_set_irq_enabled_with_callback(int_pin, GPIO_IRQ_EDGE_FALL, true, &rx_irq);
    enc28j60_receive_init(&config);

    // Initialize TCP listener
    struct tcp_pcb *tcp_pcb = tcp_new();
    assert(tcp_pcb != NULL);
    assert(tcp_bind(tcp_pcb, ip_address, listen_port) == ERR_OK);
    tcp_pcb = tcp_listen(tcp_pcb);
    assert(tcp_pcb != NULL);
    tcp_accept(tcp_pcb, accept_tcp);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (true) {
        struct pbuf *p = NULL;
        queue_try_remove(&input_q, &p);

        if (p != NULL) {
          LINK_STATS_INC(link.recv);
            if(netif.input(p, &netif) != ERR_OK) {
                pbuf_free(p);
            }
        }

        sys_check_timeouts();
    }
#pragma clang diagnostic pop
}
