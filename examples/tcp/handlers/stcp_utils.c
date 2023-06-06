#include "stcp_utils.h"
#include <stcp_conf.h>
#include <ip.h>

/** TCP checksum **/
/* 
    TCP pseudo header: 
        - 32 bit: source IP
        - 32 bit: dest IP
        - 8 bit: reserved (0)
        - 8 bit: protocol (from IP hdr)
        - 16 bit: TCP segment length (TCP header + data)
*/
uint32_t stcp_cksum(tcp_header_t *tcp_header, uint32_t sip, uint32_t dip, uint16_t tcp_segment_size)
{
    uint16_t reference_cksum = tcp_header->check;
    tcp_header->check = 0;

#ifdef NAIVE_CHECKSUM

    uint32_t cksum = 0;
    //endianness?
    cksum += U32_H16(sip) + U32_L16(sip) + U32_H16(dip) + U32_L16(dip) + HTONS(IPPROTO_TCP) + HTONS(tcp_segment_size);

    uint16_t *buffer = (uint16_t *)tcp_header;
    while (tcp_segment_size > 1)
    {
        cksum += *buffer++;
        tcp_segment_size -= sizeof(uint16_t);
    }
    if (tcp_segment_size)
    {
        cksum += *((uint8_t *)buffer);
    }

    //while (cksum >> 16) { cksum = (cksum & 0xFFFF) + (cksum >> 16); }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    cksum = ~cksum;

    //printf("TCP checksum: %x; computed TCP checksum: %x; valid: %u\n", reference_cksum, (uint16_t) cksum, (((uint16_t) cksum) == reference_cksum));
#ifdef STCP_DEBUG
    if (((uint16_t)cksum) != reference_cksum) {
        STCP_DPRINTF("TCP checksum failed: expected: %x; computed: %x\n", reference_cksum, (uint16_t) cksum);
    }
#endif

    return (((uint16_t)cksum) == reference_cksum);

#elif defined(INT32_CKSUM)
    uint64_t sum = 0;
    uint16_t len = tcp_segment_size;
    sum += U32_H16(sip) + U32_L16(sip) + U32_H16(dip) + U32_L16(dip) + HTONS(IPPROTO_TCP) + HTONS(tcp_segment_size);
    uint8_t *data = (uint8_t *)tcp_header;
    uint32_t *p = (uint32_t *)tcp_header;
    uint16_t i = 0;
    while (len >= 4)
    {
        sum = sum + p[i++];
        len -= 4;
    }
    if (len >= 2)
    {
        sum = sum + ((uint16_t *)data)[i * 4];
        len -= 2;
    }
    if (len == 1)
    {
        sum += data[len - 1];
    }

    // Fold sum into 16-bit word.
    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }

#ifdef STCP_DEBUG
    if (((uint16_t)sum) != reference_cksum) {
        STCP_DPRINTF("TCP checksum failed: expected: %x; computed: %x\n", reference_cksum, (uint16_t) cksum);
    }
#endif

    return (((uint16_t)sum) == reference_cksum);
#endif
}

void stcp_printip(int ip)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;

    printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);
}

