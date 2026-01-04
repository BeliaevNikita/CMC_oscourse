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

// MAC addresses (declared here, defined once in ethernet.c)
extern const uint8_t qemu_mac[6];
extern const uint8_t hard_code_destination_mac[6];

#define MIM_ETH_FRAME 64
#define MAX_ETH_FRAME 1518

#define ETH_MTU             1500 // Max payload size (no headers)
#define ETH_HDR_LEN         sizeof(struct eth_hdr)
#define ETH_FRAME_MAX_LEN   (ETH_HDR_LEN + ETH_MTU)

#define ETH_TYPE_IP 0x0800
#define ETH_TYPE_ARP 0x0806

#endif
