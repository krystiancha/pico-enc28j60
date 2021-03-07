#ifndef ENC28J60_ENC28J60_H
#define ENC28J60_ENC28J60_H

#include <hardware/spi.h>

/// ENC28J60 configuration
typedef struct enc28j60_config {

    /// SPI bus
    ///
    /// SPI bus which has the ENC28J60.
    /// Probably spi0 or spi1 from hardware/spi.h.
    ///
    /// The bus MUST be initialized before calling any enc28j60_* function.
    /// Use spi_init and gpio_set_function.
    spi_inst_t *spi;

    /// Chip Select pin
    ///
    /// This pin MUST be configured as output before calling any enc28j60_* function.
    /// Use gpio_init and gpio_set_dir.
    uint8_t cs_pin;

    /// Reception buffer size
    ///
    /// The buffer address space ranges from 0 to 8191 inclusive.
    /// Space from 0 to rcv_buffer_size - 1 is used as the receive buffer.
    /// Space from rcv_buffer_size to 8191 is used as the transmit buffer.
    /// rcv_buffer_size should be an even number.
    uint16_t rcv_buffer_size;

    /// MAC address of the device
    ///
    /// Example:
    /// Address 62:5E:22:07:DE:92
    /// would be set like this:
    /// { 0x62, 0x5E, 0x22, 0x07, 0xDE, 0x92 }
    uint8_t mac_address[6];

} enc28j60_config_t;

/// Returns default configuration
///
/// This configuration is not suitable to use as is.
/// It is just a starting point for your own config.
///
/// \return enc28j60_config_t instance containing default settings
enc28j60_config_t enc28j60_get_default_config();

/// Soft resets the IC
void enc28j60_reset(enc28j60_config_t *config);

/// Initializes the IC using given config object
void enc28j60_init(enc28j60_config_t *config);

/// Enables reception of packets
void enc28j60_receive_init(enc28j60_config_t *config);

/// Receives a single packet
///
/// \param next_packet pointer to a variable storing the address of the next packet;
/// it is used to set pointers for reception of the current packet;
/// the underlying value is modified to reflect the address of the next packet
/// \param buffer pointer to a buffer where the packet will be written
/// \return size of the received packet
size_t enc28j60_receive_packet(enc28j60_config_t *config, uint16_t *next_packet, uint8_t *buffer);

/// Transmits a single packet
///
/// \param buffer pointer to a buffer from where the packet will be read
/// \param len size of the packet to be transmitted
void enc28j60_transmit_packet(enc28j60_config_t *config, uint8_t *buffer, size_t len);

// --- LOW-LEVEL STUFF BELOW ---

void enc28j60_write(enc28j60_config_t *config, uint8_t instruction, uint8_t *data, size_t len);
uint8_t enc28j60_read_cr8(enc28j60_config_t *config, uint8_t address, bool skip_dummy);
uint16_t enc28j60_read_cr16(enc28j60_config_t *config, uint8_t address);
void enc28j60_write_cr8(enc28j60_config_t *config, uint8_t address, uint8_t data);
void enc28j60_write_cr16(enc28j60_config_t *config, uint8_t address, uint16_t data);
void enc28j60_bit_set(enc28j60_config_t *config, uint8_t address, uint8_t mask);
void enc28j60_bit_clear(enc28j60_config_t *config, uint8_t address, uint8_t mask);
void enc28j60_switch_bank(enc28j60_config_t *config, uint8_t bank);

// Common Control Registers
extern const uint8_t ENC28J60_EIE;
extern const uint8_t ENC28J60_EIR;
extern const uint8_t ENC28J60_ESTAT;
extern const uint8_t ENC28J60_ECON2;
extern const uint8_t ENC28J60_ECON1;

// Bank 0 Control Registers
extern const uint8_t ENC28J60_ERDPT;
extern const uint8_t ENC28J60_EWRPT;
extern const uint8_t ENC28J60_ETXST;
extern const uint8_t ENC28J60_ETXND;
extern const uint8_t ENC28J60_ERXST;
extern const uint8_t ENC28J60_ERXND;
extern const uint8_t ENC28J60_ERXRDPT;
extern const uint8_t ENC28J60_ERXWRPT;
extern const uint8_t ENC28J60_EDMAST;
extern const uint8_t ENC28J60_EDMAND;
extern const uint8_t ENC28J60_EDMADST;
extern const uint8_t ENC28J60_EDMACS;

// Bank 1 Control Registers
extern const uint8_t ENC28J60_EHT;
extern const uint8_t ENC28J60_EPMM;
extern const uint8_t ENC28J60_EPMCS;
extern const uint8_t ENC28J60_EPMO;
extern const uint8_t ENC28J60_ERXFCON;
extern const uint8_t ENC28J60_EPKTCNT;

// Bank 2 Control Registers
extern const uint8_t ENC28J60_MACON1;
extern const uint8_t ENC28J60_MACON3;
extern const uint8_t ENC28J60_MACON4;
extern const uint8_t ENC28J60_MABBIPG;
extern const uint8_t ENC28J60_MAIPG;
extern const uint8_t ENC28J60_MACLCON1;
extern const uint8_t ENC28J60_MACLCON2;
extern const uint8_t ENC28J60_MAMXFL;
extern const uint8_t ENC28J60_MICMD;
extern const uint8_t ENC28J60_MIREGADR;
extern const uint8_t ENC28J60_MIWR;
extern const uint8_t ENC28J60_MIRD;

// Bank 3 Control Registers
extern const uint8_t ENC28J60_MAADR5;
extern const uint8_t ENC28J60_MAADR6;
extern const uint8_t ENC28J60_MAADR3;
extern const uint8_t ENC28J60_MAADR4;
extern const uint8_t ENC28J60_MAADR1;
extern const uint8_t ENC28J60_MAADR2;
extern const uint8_t ENC28J60_EBSTD;
extern const uint8_t ENC28J60_EBSTCON;
extern const uint8_t ENC28J60_EBSTCS;
extern const uint8_t ENC28J60_MISTAT;
extern const uint8_t ENC28J60_EREVID;
extern const uint8_t ENC28J60_ECOCON;
extern const uint8_t ENC28J60_EFLOCON;
extern const uint8_t ENC28J60_EPAUS;

// PHY Registers
extern const uint8_t ENC28J60_PHCON1;
extern const uint8_t ENC28J60_PHSTAT1;
extern const uint8_t ENC28J60_PHID1;
extern const uint8_t ENC28J60_PHID2;
extern const uint8_t ENC28J60_PHCON2;
extern const uint8_t ENC28J60_PHSTAT2;
extern const uint8_t ENC28J60_PHIE;
extern const uint8_t ENC28J60_PHIR;
extern const uint8_t ENC28J60_PHLCON;

// Register Bits
extern const uint8_t ENC28J60_UCEN;
extern const uint8_t ENC28J60_ANDOR;
extern const uint8_t ENC28J60_CRCEN;
extern const uint8_t ENC28J60_PMEN;
extern const uint8_t ENC28J60_MPEN;
extern const uint8_t ENC28J60_HTEN;
extern const uint8_t ENC28J60_MCEN;
extern const uint8_t ENC28J60_BCEN;

extern const uint8_t ENC28J60_TXPAUS;
extern const uint8_t ENC28J60_RXPAUS;
extern const uint8_t ENC28J60_PASSALL;
extern const uint8_t ENC28J60_MARXEN;

extern const uint8_t ENC28J60_PADCFG_64;
extern const uint8_t ENC28J60_PADCFG_NO;
extern const uint8_t ENC28J60_PADCFG_VLAN;
extern const uint8_t ENC28J60_PADCFG_60;
extern const uint8_t ENC28J60_TXCRCEN;
extern const uint8_t ENC28J60_PHDREN;
extern const uint8_t ENC28J60_HFRMEN;
extern const uint8_t ENC28J60_FRMLNEN;
extern const uint8_t ENC28J60_FULDPX;

extern const uint16_t ENC28J60_FRCLNK;
extern const uint16_t ENC28J60_TXDIS;
extern const uint16_t ENC28J60_JABBER;
extern const uint16_t ENC28J60_HDLDIS;

extern const uint8_t ENC28J60_INTIE;
extern const uint8_t ENC28J60_PKTIE;
extern const uint8_t ENC28J60_DMAIE;
extern const uint8_t ENC28J60_LINKIE;
extern const uint8_t ENC28J60_TXIE;
extern const uint8_t ENC28J60_TXERIE;
extern const uint8_t ENC28J60_RXERIE;

extern const uint8_t ENC28J60_TXRST;
extern const uint8_t ENC28J60_RXRST;
extern const uint8_t ENC28J60_DMAST;
extern const uint8_t ENC28J60_CSUMEN;
extern const uint8_t ENC28J60_TXRTS;
extern const uint8_t ENC28J60_RXEN;

extern const uint8_t ENC28J60_PKTIF;
extern const uint8_t ENC28J60_DMAIF;
extern const uint8_t ENC28J60_LINKIF;
extern const uint8_t ENC28J60_TXIF;
extern const uint8_t ENC28J60_TXERIF;
extern const uint8_t ENC28J60_RXERIF;

extern const uint8_t ENC28J60_AUTOINC;
extern const uint8_t ENC28J60_PKTDEC;
extern const uint8_t ENC28J60_PWRSV;
extern const uint8_t ENC28J60_VRPS;

extern const uint8_t ENC28J60_DEFER;

extern const uint8_t ENC28J60_MIIRD;

extern const uint8_t ENC28J60_INT;
extern const uint8_t ENC28J60_BUFER;
extern const uint8_t ENC28J60_LATECOL;
extern const uint8_t ENC28J60_RXBUSY;
extern const uint8_t ENC28J60_TXABRT;
extern const uint8_t ENC28J60_CLKRDY;

// Instructions
extern const uint8_t ENC28J60_RCR;  // Read Control Register
extern const uint8_t ENC28J60_RBM;  // Read Buffer Memory
extern const uint8_t ENC28J60_WCR;  // Write Control Register
extern const uint8_t ENC28J60_WBM;  // Write Buffer Memory
extern const uint8_t ENC28J60_BFS;  // Bit Field Set
extern const uint8_t ENC28J60_BFC;  // Bit Field Clear
extern const uint8_t ENC28J60_SRC;  // System Reset Command (Soft Reset)

extern const uint8_t ENC28J60_BM_ARG;  // RBM/WBM Argument
extern const uint8_t ENC28J60_SRC_ARG;  // SRC Argument

#endif //ENC28J60_ENC28J60_H
