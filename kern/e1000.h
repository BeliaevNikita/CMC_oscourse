#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_NU_DESC     64      // Number of descriptors (RX or TX)
#define E1000_BUFFER_SIZE 1518    // Same as ethernet packet size

// TX Descriptor
struct tx_desc {
    uint64_t buf_addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t css;
    uint8_t status;
    uint16_t special;
};

// RX Descriptor
struct rx_desc {
    uint64_t buf_addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
};

// Base mmio address
extern volatile uint32_t *phy_mmio_addr;

// TX Descriptor Registers
#define E1000_TCTL  0x00400 // Control Register - RW
#define E1000_TIPG  0x00410 // Inter-packet Gap - RW
#define E1000_TDBAL 0x03800 // Base Address Low - RW
#define E1000_TDBAH 0x03804 // Base Address High - RW
#define E1000_TDLEN 0x03808 // Length - RW
#define E1000_TDH   0x03810 // Head - RW
#define E1000_TDT   0x03818 // Tail - RW

// Transmit control
#define E1000_TCTL_EN   0x00000002  // Enable TX
#define E1000_TCTL_PSP  0x00000008  // Pad Short Packets
#define E1000_TCTL_CT   0x00000ff0  // Collision Threshold
#define E1000_TCTL_COLD 0x1000000   // Collision Distance

// TX Descriptor bit definitions
#define E1000_TXD_STAT_DD 0x00000001    // Descriptor Done
#define E1000_TXD_CMD_RS  0x00000008    // Report Status
#define E1000_TXD_CMD_EOP 0x01000000    // End of Packet

// Rx Desc Registers
#define E1000_RCTL  0x00100 // RX Control - RW
#define E1000_RDBAL 0x02800 // Base Address Low - RW
#define E1000_RDBAH 0x02804 // Base Address High - RW
#define E1000_RDLEN 0x02808 // Length - RW
#define E1000_RDH   0x02810 // Head - RW
#define E1000_RDT   0x02818 // Tail - RW
#define E1000_MTA   0x5200  // Multicast Table Array - RW Array

// Receive Control
#define E1000_RCTL_EN  0x00000002   // Enable RX
#define E1000_RCTL_BAM 0x00008000   // Broadcast Enable
#define E1000_RCTL_CRC 0x04000000   // Strip Ethernet CRC

// RX Descriptor bit definitions
#define E1000_RXD_STAT_DD 0x01  // Descriptor Done

// General Registers
#define E1000_RAL 0x05400    // Receive Address Low - RW Array
#define E1000_RAH 0x05404    // Receive Address High - RW Array


extern struct tx_desc tx_desc_table[E1000_NU_DESC];
extern struct rx_desc rx_desc_table[E1000_NU_DESC];
extern char tx_buf[E1000_NU_DESC][E1000_BUFFER_SIZE];
extern char rx_buf[E1000_NU_DESC][E1000_BUFFER_SIZE];


int e1000_attach(struct pci_func *pcif);
int e1000_transmit(const char *buf, unsigned len);
int e1000_receive(char *buf);

#endif // JOS_KERN_E1000_H
