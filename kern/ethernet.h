#ifndef JOS_KERN_ETH_H
#define JOS_KERN_ETH_H

#include <inc/types.h>
#include <kern/e1000.h>

struct eth_hdr {
    uint8_t eth_destination_mac[6];
    uint8_t eth_source_mac[6];
    uint16_t eth_type;
} __attribute__((packed));

int eth_send(struct eth_hdr *hdr, void *data, size_t len);
int eth_recv(void *data);

//52:54:00:12:34:56
const char qemu_mac[6] = {0x52, 0x54, 0x0, 0x12, 0x34, 0x56};
char hard_code_destination_mac[6] = {0x3a, 0xbe, 0x6d, 0xa0, 0xaf, 0x00};

#define MIM_ETH_FRAME 64
#define MAX_ETH_FRAME 1518

const size_t ETH_MTU = 1500; // Max payload size (no headers)
const size_t ETH_HDR_LEN = sizeof(struct eth_hdr);
const size_t ETH_FRAME_MAX_LEN = ETH_HDR_LEN + ETH_MTU;

#define ETH_TYPE_IP 0x0800
#define ETH_TYPE_ARP 0x0806

#endif
