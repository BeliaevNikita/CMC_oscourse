#include <kern/ethernet.h>
#include <kern/e1000.h>
#include <kern/inet.h>
#include <kern/arp.h>

#include <inc/string.h>
#include <inc/error.h>
#include <inc/assert.h>

//52:54:00:12:34:56
const uint8_t qemu_mac[6] = {0x52, 0x54, 0x0, 0x12, 0x34, 0x56};
//3a:be:6d:a0:af:00
const uint8_t hard_code_destination_mac[6] = {0x3a, 0xbe, 0x6d, 0xa0, 0xaf, 0x00};

int
eth_send(struct eth_hdr *hdr, void *data, size_t len) {
    // if (trace_packet_processing) cprintf("Sending Ethernet packet\n");
    assert(len <= ETH_MTU);

    // force substitute just in case
    memcpy((void*) hdr->eth_source_mac, qemu_mac, sizeof(hdr->eth_source_mac));
    // memcpy((void*) hdr->eth_destination_mac, hard_code_destination_mac, sizeof(hdr->eth_destination_mac));
    if (hdr->eth_type == htons(ETH_TYPE_IP)) {
        struct ip_hdr *ip_header = &(((struct ip_pkt *) data)->hdr);
        memcpy(hdr->eth_destination_mac, get_mac_by_ip(ip_header->ip_destination_address), 6);
    }

    char buf[ETH_FRAME_MAX_LEN];
    hdr->eth_type = htons(hdr->eth_type);
    memcpy((void *) buf, (void *) hdr, sizeof(*hdr));
    memcpy((void *) buf + sizeof(*hdr), data, len);

    return e1000_transmit(buf, sizeof(*hdr) + len);
}


int
eth_recv(void *data) {
    char buf[E1000_BUFFER_SIZE];

    int size = e1000_receive(buf);
    if (size <= 0) {
        return size;
    }

    // Get the Ethernet header
    struct eth_hdr hdr = {0};
    memcpy((void *) &hdr, (void *) buf, sizeof(hdr));
    // The only field that needs to endianness change
    hdr.eth_type = ntohs(hdr.eth_type);

    // Get the payload
    memcpy(data, (void *) buf + sizeof(hdr), size - sizeof(hdr));

    // Dispatch to higher level protocols
    switch (hdr.eth_type) {
        case ETH_TYPE_IP: {
            if (ip_recv(data) < 0) {
                return -1;
            }
        }
        case ETH_TYPE_ARP: {
            if (arp_resolve(data) < 0) {
                return -1;
            }
        }
        default: {
            // return -E_BAD_ETH_TYPE;
        }
    }

    return size; // TODO: maybe return payload size
}

