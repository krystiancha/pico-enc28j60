#include <stdio.h>
#include <pico/stdlib.h>

#include <pico/enc28j60/enc28j60.h>

void enc28j60_init(enc28j60_t *self) {
	// Soft reset
	enc28j60_write(self, ENC28J60_SRC | ENC28J60_SRC_ARG, NULL, 0);
	sleep_ms(1);  // Errata issue 2

	// LED setup
	enc28j60_write_phy(self, ENC28J60_PHLCON, 0x3476);

	uint8_t prev_bank = enc28j60_switch_bank(self, 0);

	// MAC setup
	enc28j60_write_cr16(self, ENC28J60_ERXST, 0);  // Start receive buffer in 0 as per errata issue 5
	enc28j60_write_cr16(self, ENC28J60_ERXND, ENC28J60_RCV_BUFFER_SIZE - 1);
	enc28j60_write_cr16(self, ENC28J60_ERXRDPT, 0);

	enc28j60_switch_bank(self, 2);
	enc28j60_write_cr8(self, ENC28J60_MACON1, ENC28J60_MARXEN);
	enc28j60_write_cr8(self, ENC28J60_MACON3, ENC28J60_PADCFG_60 | ENC28J60_TXCRCEN | ENC28J60_FRMLNEN);
	enc28j60_write_cr8(self, ENC28J60_MACON4, ENC28J60_DEFER);
	enc28j60_write_cr16(self, ENC28J60_MAMXFL, 1518);
	enc28j60_write_cr8(self, ENC28J60_MABBIPG, 0x12);
	enc28j60_write_cr16(self, ENC28J60_MAIPG, 0x0C12);

	enc28j60_switch_bank(self, 3);
	enc28j60_write_cr16(self, ENC28J60_MAADR1, self->mac_address[0]);
	enc28j60_write_cr16(self, ENC28J60_MAADR2, self->mac_address[1]);
	enc28j60_write_cr16(self, ENC28J60_MAADR3, self->mac_address[2]);
	enc28j60_write_cr16(self, ENC28J60_MAADR4, self->mac_address[3]);
	enc28j60_write_cr16(self, ENC28J60_MAADR5, self->mac_address[4]);
	enc28j60_write_cr16(self, ENC28J60_MAADR6, self->mac_address[5]);

	// PHY setup
	enc28j60_write_phy(self, ENC28J60_PHCON2, ENC28J60_HDLDIS);	 // Disable loopback as per errata issue 9

	// Disable all filters
	enc28j60_switch_bank(self, 1);
	enc28j60_write_cr16(self, ENC28J60_ERXFCON, 0);

	// Enable reception
	enc28j60_bit_set(self, ENC28J60_ECON1, ENC28J60_RXEN);

	enc28j60_switch_bank(self, prev_bank);
}

void enc28j60_transfer_init(enc28j60_t *self) {
	uint8_t prev_bank = enc28j60_switch_bank(self, 0);
	enc28j60_write_cr16(self, ENC28J60_ETXST, ENC28J60_RCV_BUFFER_SIZE);
	enc28j60_write_cr16(self, ENC28J60_ETXND, ENC28J60_RCV_BUFFER_SIZE);
	enc28j60_write_cr16(self, ENC28J60_EWRPT, ENC28J60_RCV_BUFFER_SIZE);

	uint8_t control = 0;
	enc28j60_write(self, ENC28J60_WBM | ENC28J60_BM_ARG, &control, 1);

	enc28j60_switch_bank(self, prev_bank);
}

void enc28j60_transfer_write(enc28j60_t *self, uint8_t *payload, size_t len) {
	uint8_t prev_bank = enc28j60_switch_bank(self, 0);

	uint16_t tx_buffer_end = enc28j60_read_cr16(self, ENC28J60_ETXND);
	enc28j60_write(self, ENC28J60_WBM | ENC28J60_BM_ARG, payload, len);
	enc28j60_write_cr16(self, ENC28J60_ETXND, tx_buffer_end + len);

	enc28j60_switch_bank(self, prev_bank);
}

void enc28j60_transfer_send(enc28j60_t *self) {
	// Reset transmission logic, errata issue 12
	enc28j60_bit_set(self, ENC28J60_ECON1, ENC28J60_TXRST);
	enc28j60_bit_clear(self, ENC28J60_ECON1, ENC28J60_TXRST);

	enc28j60_bit_set(self, ENC28J60_ECON1, ENC28J60_TXRTS);

	while (enc28j60_read_cr8(self, ENC28J60_ECON1, false) & ENC28J60_TXRTS) {
		sleep_us(1);
	}
}

void enc28j60_transfer_status(enc28j60_t *self, uint8_t *status) {
	if (status == NULL) {
		return;
	}

	uint8_t prev_bank = enc28j60_switch_bank(self, 0);
	enc28j60_write_cr16(self, ENC28J60_ERDPT, enc28j60_read_cr16(self, ENC28J60_ETXND) + 1);
	enc28j60_read(self, ENC28J60_RBM | ENC28J60_BM_ARG, status, 7);
	enc28j60_switch_bank(self, prev_bank);
}

uint16_t enc28j60_receive_init(enc28j60_t *self) {
	struct {
		uint16_t next_packet;
		uint16_t byte_count;
		uint16_t status;
	} header;
	uint8_t prev_bank = enc28j60_switch_bank(self, 0);
	enc28j60_write_cr16(self, ENC28J60_ERDPT, self->next_packet);
	enc28j60_read(self, ENC28J60_RBM | ENC28J60_BM_ARG, (uint8_t *) &header, 6);

	self->next_packet = header.next_packet;

	enc28j60_switch_bank(self, prev_bank);

	return (header.status & 0x80) ? header.byte_count - 4 : 0;
}

void enc28j60_receive_read(enc28j60_t *self, uint8_t *payload, size_t len) {
	enc28j60_read(self, ENC28J60_RBM | ENC28J60_BM_ARG, payload, len);
}

void enc28j60_receive_ack(enc28j60_t *self) {
	uint32_t crc;
	enc28j60_read(self, ENC28J60_RBM | ENC28J60_BM_ARG, (uint8_t *) &crc, 4);

	enc28j60_bit_set(self, ENC28J60_ECON2, ENC28J60_PKTDEC);

	// Free buffer & Errata issue 14
	uint8_t prev_bank = enc28j60_switch_bank(self, 0);
	if (self->next_packet == enc28j60_read_cr8(self, ENC28J60_ERXST, false)) {
		enc28j60_write_cr16(self, ENC28J60_ERXRDPT, enc28j60_read_cr16(self, ENC28J60_ERXND));
	} else {
		enc28j60_write_cr16(self, ENC28J60_ERXRDPT, self->next_packet - 1);
	}

	enc28j60_switch_bank(self, prev_bank);
}

void enc28j60_interrupts(enc28j60_t *self, uint8_t flags) {
	enc28j60_bit_clear(self, ENC28J60_EIR, flags);
	enc28j60_write_cr8(self, ENC28J60_EIE, flags | ENC28J60_INTIE);
}

void enc28j60_isr_begin(enc28j60_t *self) {
	enc28j60_bit_clear(self, ENC28J60_EIE, ENC28J60_INTIE);
}

void enc28j60_isr_end(enc28j60_t *self) {
	enc28j60_bit_set(self, ENC28J60_EIE, ENC28J60_INTIE);
}

uint8_t enc28j60_interrupt_flags(enc28j60_t *self) {
	uint8_t flags = enc28j60_read_cr8(self, ENC28J60_EIR, false);

	// Errata
	uint8_t prev_bank = enc28j60_switch_bank(self, 1);
	uint8_t packet_count = enc28j60_read_cr8(self, ENC28J60_EPKTCNT, false);
	if (packet_count) {
		flags |= ENC28J60_PKTIF;
	}

	enc28j60_switch_bank(self, prev_bank);

	return flags;
}

void enc28j60_interrupt_clear(enc28j60_t *self, uint8_t flags) {
	if (!flags) {
		flags = ENC28J60_PKTIF | ENC28J60_DMAIF | ENC28J60_LINKIF | ENC28J60_TXIF | ENC28J60_TXERIF | ENC28J60_RXERIF;
	}

	enc28j60_bit_clear(self, ENC28J60_EIR, flags);
}

void enc28j60_read(enc28j60_t *config, uint8_t instruction, uint8_t *data, size_t len) {
	if (config->critical_section != NULL) {
		critical_section_enter_blocking(config->critical_section);
	}
	gpio_put(config->cs_pin, 0);
	spi_write_blocking(config->spi, &instruction, 1);
	spi_read_blocking(config->spi, 0, data, len);
	gpio_put(config->cs_pin, 1);
	if (config->critical_section != NULL) {
		critical_section_exit(config->critical_section);
	}
}

void enc28j60_write(enc28j60_t *config, uint8_t instruction, uint8_t *data, size_t len) {
	if (config->critical_section != NULL) {
		critical_section_enter_blocking(config->critical_section);
	}
	gpio_put(config->cs_pin, 0);
	spi_write_blocking(config->spi, &instruction, 1);
	spi_write_blocking(config->spi, data, len);
	gpio_put(config->cs_pin, 1);
	if (config->critical_section != NULL) {
		critical_section_exit(config->critical_section);
	}
}

uint8_t enc28j60_read_cr8(enc28j60_t *config, uint8_t address, bool skip_dummy) {
	uint16_t data;
	enc28j60_read(config, ENC28J60_RCR | address, (uint8_t *) &data, skip_dummy ? 2 : 1);

	return (uint8_t) data;
}

uint16_t enc28j60_read_cr16(enc28j60_t *config, uint8_t address) {
	uint16_t data;
	enc28j60_read(config, ENC28J60_RCR | address, (uint8_t *) &data, 1);
	enc28j60_read(config, ENC28J60_RCR | (address + 1), (uint8_t *) (&data) + 1, 1);

	return data;
}

void enc28j60_write_cr8(enc28j60_t *config, uint8_t address, uint8_t data) {
	enc28j60_write(config, ENC28J60_WCR | address, &data, 1);
}

void enc28j60_write_cr16(enc28j60_t *config, uint8_t address, uint16_t data) {
	enc28j60_write(config, ENC28J60_WCR | address, (uint8_t *) &data, 1);
	enc28j60_write(config, ENC28J60_WCR | (address + 1), (uint8_t *) (&data) + 1, 1);
}

void enc28j60_bit_set(enc28j60_t *config, uint8_t address, uint8_t mask) {
	enc28j60_write(config, ENC28J60_BFS | address , &mask, 1);
}

void enc28j60_bit_clear(enc28j60_t *config, uint8_t address, uint8_t mask) {
	enc28j60_write(config, ENC28J60_BFC | address , &mask, 1);
}

uint8_t enc28j60_switch_bank(enc28j60_t *config, uint8_t bank) {
	uint8_t prev_bank = enc28j60_read_cr8(config, ENC28J60_ECON1, false) & 0x03;

	enc28j60_bit_clear(config, ENC28J60_ECON1, 0x03);
	enc28j60_bit_set(config, ENC28J60_ECON1, bank & 0x03); // & 0x03 in case of a bad argument

	return prev_bank;
}

uint16_t enc28j60_read_phy(enc28j60_t *config, uint8_t address) {
	uint8_t prev_bank = enc28j60_switch_bank(config, 2);

	enc28j60_write_cr8(config, ENC28J60_MIREGADR, address);
	enc28j60_bit_set(config, ENC28J60_MICMD, ENC28J60_MIIRD);
	sleep_ms(1);  // TODO: polling
	uint16_t data = (uint16_t) enc28j60_read_cr8(config, ENC28J60_MIRD, true) | ((uint16_t) enc28j60_read_cr8(config, ENC28J60_MIRD + 1, true) << 8);
	enc28j60_bit_clear(config, ENC28J60_MICMD, ENC28J60_MIIRD);

	enc28j60_switch_bank(config, prev_bank);

	return data;
}

void enc28j60_write_phy(enc28j60_t *config, uint8_t address, uint16_t data) {
	uint8_t prev_bank = enc28j60_switch_bank(config, 2);
	enc28j60_write_cr8(config, ENC28J60_MIREGADR, address);
	enc28j60_write_cr16(config, ENC28J60_MIWR, data);

	sleep_ms(1);  // TODO: polling

	enc28j60_switch_bank(config, prev_bank);
}

// Reception buffer size
//
// The buffer address space ranges from 0 to 8191 (inclusive).
// Space from 0 to rcv_buffer_size - 1 (inclusive) is used as the receive buffer.
// Space from rcv_buffer_size to 8191 (inclusive) is used as the transmit buffer.
// rcv_buffer_size should be an even number as recommended in the datasheet.
// For the transmit buffer to only hold one packet set rcv_buffer_size to 6666 (lol).
// This would leave 1526 bytes for the transmit buffer,
// which is 1518 (max frame size) + 1 (control byte) + 8 (status vector)
const uint16_t ENC28J60_RCV_BUFFER_SIZE = 6666;

// Instructions
const uint8_t ENC28J60_RCR = 0x00; // Read Control Register
const uint8_t ENC28J60_RBM = 0x20; // Read Buffer Memory
const uint8_t ENC28J60_WCR = 0x40; // Write Control Register
const uint8_t ENC28J60_WBM = 0x60; // Write Buffer Memory
const uint8_t ENC28J60_BFS = 0x80; // Bit Field Set
const uint8_t ENC28J60_BFC = 0xA0; // Bit Field Clear
const uint8_t ENC28J60_SRC = 0xE0; // System Reset Command (Soft Reset)

const uint8_t ENC28J60_BM_ARG = 0x1A; // RBM/WBM Argument
const uint8_t ENC28J60_SRC_ARG = 0x1F; // SRC Argument

// Common Control Registers
const uint8_t ENC28J60_EIE = 0x1B;
const uint8_t ENC28J60_EIR = 0x1C;
const uint8_t ENC28J60_ESTAT = 0x1D;
const uint8_t ENC28J60_ECON2 = 0x1E;
const uint8_t ENC28J60_ECON1 = 0x1F;

// Bank 0 Control Registers
const uint8_t ENC28J60_ERDPT = 0x00;
const uint8_t ENC28J60_EWRPT = 0x02;
const uint8_t ENC28J60_ETXST = 0x04;
const uint8_t ENC28J60_ETXND = 0x06;
const uint8_t ENC28J60_ERXST = 0x08;
const uint8_t ENC28J60_ERXND = 0x0A;
const uint8_t ENC28J60_ERXRDPT = 0x0C;
const uint8_t ENC28J60_ERXWRPT = 0x0E;
const uint8_t ENC28J60_EDMAST = 0x10;
const uint8_t ENC28J60_EDMAND = 0x12;
const uint8_t ENC28J60_EDMADST = 0x14;
const uint8_t ENC28J60_EDMACS = 0x06;

// Bank 1 Control Registers
const uint8_t ENC28J60_EHT = 0x00;
const uint8_t ENC28J60_EPMM = 0x08;
const uint8_t ENC28J60_EPMCS = 0x10;
const uint8_t ENC28J60_EPMO = 0x14;
const uint8_t ENC28J60_ERXFCON = 0x18;
const uint8_t ENC28J60_EPKTCNT = 0x19;

// Bank 2 Control Registers
const uint8_t ENC28J60_MACON1 = 0x00;
const uint8_t ENC28J60_MACON3 = 0x02;
const uint8_t ENC28J60_MACON4 = 0x03;
const uint8_t ENC28J60_MABBIPG = 0x04;
const uint8_t ENC28J60_MAIPG = 0x06;
const uint8_t ENC28J60_MACLCON1 = 0x08;
const uint8_t ENC28J60_MACLCON2 = 0x09;
const uint8_t ENC28J60_MAMXFL = 0x0A;
const uint8_t ENC28J60_MICMD = 0x12;
const uint8_t ENC28J60_MIREGADR = 0x14;
const uint8_t ENC28J60_MIWR = 0x16;
const uint8_t ENC28J60_MIRD = 0x18;

// Bank 3 Control Registers
const uint8_t ENC28J60_MAADR5 = 0x00;
const uint8_t ENC28J60_MAADR6 = 0x01;
const uint8_t ENC28J60_MAADR3 = 0x02;
const uint8_t ENC28J60_MAADR4 = 0x03;
const uint8_t ENC28J60_MAADR1 = 0x04;
const uint8_t ENC28J60_MAADR2 = 0x05;
const uint8_t ENC28J60_EBSTD = 0x06;
const uint8_t ENC28J60_EBSTCON = 0x07;
const uint8_t ENC28J60_EBSTCS = 0x08;
const uint8_t ENC28J60_MISTAT = 0x0A;
const uint8_t ENC28J60_EREVID = 0x12;
const uint8_t ENC28J60_ECOCON = 0x15;
const uint8_t ENC28J60_EFLOCON = 0x17;
const uint8_t ENC28J60_EPAUS = 0x18;

// PHY Registers
const uint8_t ENC28J60_PHCON1 = 0x00;
const uint8_t ENC28J60_PHSTAT1 = 0x01;
const uint8_t ENC28J60_PHID1 = 0x02;
const uint8_t ENC28J60_PHID2 = 0x03;
const uint8_t ENC28J60_PHCON2 = 0x10;
const uint8_t ENC28J60_PHSTAT2 = 0x11;
const uint8_t ENC28J60_PHIE = 0x12;
const uint8_t ENC28J60_PHIR = 0x13;
const uint8_t ENC28J60_PHLCON = 0x14;

// Register Bits
const uint8_t ENC28J60_UCEN = 0x80;
const uint8_t ENC28J60_ANDOR = 0x40;
const uint8_t ENC28J60_CRCEN = 0x20;
const uint8_t ENC28J60_PMEN = 0x10;
const uint8_t ENC28J60_MPEN = 0x08;
const uint8_t ENC28J60_HTEN = 0x04;
const uint8_t ENC28J60_MCEN = 0x02;
const uint8_t ENC28J60_BCEN = 0x01;

const uint8_t ENC28J60_TXPAUS = 0x08;
const uint8_t ENC28J60_RXPAUS = 0x04;
const uint8_t ENC28J60_PASSALL = 0x02;
const uint8_t ENC28J60_MARXEN = 0x01;

const uint8_t ENC28J60_PADCFG_64 = 0xE0;
const uint8_t ENC28J60_PADCFG_NO = 0xC0;
const uint8_t ENC28J60_PADCFG_VLAN = 0xA0;
const uint8_t ENC28J60_PADCFG_60 = 0x20;
const uint8_t ENC28J60_TXCRCEN = 0x10;
const uint8_t ENC28J60_PHDREN = 0x08;
const uint8_t ENC28J60_HFRMEN = 0x04;
const uint8_t ENC28J60_FRMLNEN = 0x02;
const uint8_t ENC28J60_FULDPX = 0x01;

const uint16_t ENC28J60_FRCLNK = 0x0400;
const uint16_t ENC28J60_TXDIS = 0x0200;
const uint16_t ENC28J60_JABBER = 0x0400;
const uint16_t ENC28J60_HDLDIS = 0x0100;

const uint8_t ENC28J60_INTIE = 0x80;
const uint8_t ENC28J60_PKTIE = 0x40;
const uint8_t ENC28J60_DMAIE = 0x20;
const uint8_t ENC28J60_LINKIE = 0x10;
const uint8_t ENC28J60_TXIE = 0x08;
const uint8_t ENC28J60_TXERIE = 0x02;
const uint8_t ENC28J60_RXERIE = 0x01;

const uint8_t ENC28J60_TXRST = 0x80;
const uint8_t ENC28J60_RXRST = 0x40;
const uint8_t ENC28J60_DMAST = 0x20;
const uint8_t ENC28J60_CSUMEN = 0x10;
const uint8_t ENC28J60_TXRTS = 0x08;
const uint8_t ENC28J60_RXEN = 0x04;

const uint8_t ENC28J60_PKTIF = 0x40;
const uint8_t ENC28J60_DMAIF = 0x20;
const uint8_t ENC28J60_LINKIF = 0x10;
const uint8_t ENC28J60_TXIF = 0x08;
const uint8_t ENC28J60_TXERIF = 0x02;
const uint8_t ENC28J60_RXERIF = 0x01;

const uint8_t ENC28J60_AUTOINC = 0x80;
const uint8_t ENC28J60_PKTDEC = 0x40;
const uint8_t ENC28J60_PWRSV = 0x20;
const uint8_t ENC28J60_VRPS = 0x08;

const uint8_t ENC28J60_DEFER = 0x40;

const uint8_t ENC28J60_MIIRD = 0x01;

const uint8_t ENC28J60_INT = 0x80;
const uint8_t ENC28J60_BUFER = 0x40;
const uint8_t ENC28J60_LATECOL = 0x10;
const uint8_t ENC28J60_RXBUSY = 0x04;
const uint8_t ENC28J60_TXABRT = 0x02;
const uint8_t ENC28J60_CLKRDY = 0x01;
