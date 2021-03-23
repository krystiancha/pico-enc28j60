#ifndef ENC28J60_H
#define ENC28J60_H

#include <hardware/spi.h>
#include <pico/critical_section.h>

/// Extract the value of a status bit from a packet transmit status vector
///
/// Will return non-zero value if the bit is set, zero otherwise.
/// \param status buffer containing the status vector
/// \param bit bit index
#define ENC28J60_TX_STATUS_BIT(status, bit) (status[bit / 8] & 1 << (bit % 8))

/// ENC28J60 configuration
struct enc28j60 {

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

	/// MAC address of the device
	///
	/// Example:
	/// Address 1F:63:88:BC:39:87
	/// would be set like this:
	/// { 0x1F, 0x63, 0x88, 0xBC, 0x39, 0x87 }
	uint8_t mac_address[6];

	/// Critical section for IRQ safe mutual exclusion of the enc28j60 device
	///
	/// If critical_section is set to non-NULL value, the critical section is entered (blocking) for the time of
	/// executing every SPI command.
	///
	/// If you use this library in a way that execution of a command can be interrupted (for example: main loop and
	/// interrupt service routine), then set critical_section to an initialized critical section object (use
	/// critical_section_init). Otherwise, remember to set this to NULL.
	critical_section_t *critical_section;

	/// Address of the next packet in the receive buffer
	///
	/// You shouldn't have to modify this, it is managed by the library.
	uint16_t next_packet;

};

/// Soft reset, initialize and enable packet reception
void enc28j60_init(struct enc28j60 *self);

/// Start the process of transmitting a single packet
void enc28j60_transfer_init(struct enc28j60 *self);

/// Write data to the transmit buffer of the IC
///
/// Subsequent calls to this function will append data to the buffer.
/// This way the packet can be written in chunks.
/// Write at most 1526 bytes, because that is the size of the transmit buffer.
/// \param payload pointer to the application buffer
/// \param len length of the data to be written
void enc28j60_transfer_write(struct enc28j60 *self, uint8_t *payload, size_t len);

/// Transmits the packet that is currently in the transmit buffer
///
/// This function blocks until the packet is transmitted or aborted due to an error.
void enc28j60_transfer_send(struct enc28j60 *self);

/// Retrieves the status vector of last transmitted packet
///
/// This library provides the ENC28J60_TX_STATUS_BIT macro for convenient access to single bits of the status vector.
///
/// \param status a seven byte buffer, where the status will be written to
void enc28j60_transfer_status(struct enc28j60 *self, uint8_t *status);

/// Start the process of receiving the next single packet from the receive buffer
/// \return packet size in bytes
uint16_t enc28j60_receive_init(struct enc28j60 *self);

/// Read data from the receive buffer of the IC
///
/// Subsequent calls to this function allow to read the packet in chunks.
/// Reading more data than the length returned from enc28j60_receive_init will probably ruin your day.
/// \param payload a buffer to copy the data to
/// \param len amount of bytes to read
void enc28j60_receive_read(struct enc28j60 *self, uint8_t *payload, size_t len);

/// End the packet reception process and free part of the receive buffer of the IC
void enc28j60_receive_ack(struct enc28j60 *self);

/// Enable or disable interrupts on the INT pin of the IC
///
/// Interrupts specified in the flags argument will be enabled, the rest of them will be disabled.
///
/// \param flags mask built from ENC28J60_{PKTIE,DMAIE,LINKIE,TXIE,TXERIE,RXERIE}
void enc28j60_interrupts(struct enc28j60 *self, uint8_t flags);

/// Clears the EIE.INTIE bit
///
/// Call at the beginning of the interrupt service routine to prevent missing a falling edge.
void enc28j60_isr_begin(struct enc28j60 *self);

/// Sets the EIE.INTIE bit
///
/// Call at the beginning of the interrupt service routine to prevent missing a falling edge.
void enc28j60_isr_end(struct enc28j60 *self);

/// Read the interrupt flags
///
/// Call in the interrupt service routine to find out the reason for the interrupt.
/// \return mask built from ENC28J60_{PKTIF,DMAIF,LINKIF,TXIF,TXERIF,RXERIF}
uint8_t enc28j60_interrupt_flags(struct enc28j60 *self);

/// Clears interrupt flags
///
/// \param flags mask built from interrupt flags (see enc28j60_interrupt_flags); if 0 then clears all flags
void enc28j60_interrupt_clear(struct enc28j60 *self, uint8_t flags);

// --- LOW-LEVEL STUFF BELOW --- you probably won't need this

void enc28j60_read(struct enc28j60 *config, uint8_t instruction, uint8_t *data, size_t len);
void enc28j60_write(struct enc28j60 *config, uint8_t instruction, uint8_t *data, size_t len);
uint8_t enc28j60_read_cr8(struct enc28j60 *config, uint8_t address, bool skip_dummy);
uint16_t enc28j60_read_cr16(struct enc28j60 *config, uint8_t address);
void enc28j60_write_cr8(struct enc28j60 *config, uint8_t address, uint8_t data);
void enc28j60_write_cr16(struct enc28j60 *config, uint8_t address, uint16_t data);
void enc28j60_bit_set(struct enc28j60 *config, uint8_t address, uint8_t mask);
void enc28j60_bit_clear(struct enc28j60 *config, uint8_t address, uint8_t mask);
uint8_t enc28j60_switch_bank(struct enc28j60 *config, uint8_t bank);
uint16_t enc28j60_read_phy(struct enc28j60 *config, uint8_t address);
void enc28j60_write_phy(struct enc28j60 *config, uint8_t address, uint16_t data);

extern const uint16_t ENC28J60_RCV_BUFFER_SIZE;  // Reception buffer size

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

#endif //ENC28J60_H
