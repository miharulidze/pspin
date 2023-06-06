#include "pcap.h"
#include "ethernet.h"
#include "ip.h"
#include "tcp.h"

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#define IP_STR_LEN 1024

typedef struct eth_ip_header
{
    ether_header_t eth;
    ip_header_t ip;
} __attribute__((__packed__)) eth_ip_header_t;

void int2ip(uint32_t ip, char *buff, size_t len)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    snprintf(buff, len, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
}

uint16_t tcp_checksum(uint8_t* tcp_segment, uint32_t tcp_segment_size, uint32_t ip_src, uint32_t ip_dst)
{
    uint32_t cksum = 0;
    //endianness?
    cksum += U32_H16(ip_src) + U32_L16(ip_src) + U32_H16(ip_dst) + U32_L16(ip_dst) + HTONS(IPPROTO_TCP) + HTONS(tcp_segment_size);

    uint16_t *buffer = (uint16_t *)tcp_segment;
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

    return (uint16_t) cksum;
}

static bool tcp_is_valid(uint8_t *ip_pkt, bool fix, uint8_t *syn, uint8_t *ack, uint8_t *fin)
{

    ip_header_t *ip_hdr = (ip_header_t *)(ip_pkt);
    uint16_t ip_hdr_len = ip_hdr->ihl * sizeof(uint32_t);

    tcp_header_t *tcp_header = (tcp_header_t *)(ip_pkt + ip_hdr_len);
    uint16_t tcp_segment_size = NTOHS(ip_hdr->tot_len) - ip_hdr_len;

    printf("total_length offset: %u\n", offsetof(ip_header_t, tot_len));
    printf("IP total length: %hu; IP header len: %hu; TCP segment len: %hu\n", NTOHS(ip_hdr->tot_len), ip_hdr_len, tcp_segment_size);
    //printf("TCP seqno: %u\n", tcp_header->seq);

    uint32_t sip = ip_hdr->saddr;
    uint32_t dip = ip_hdr->daddr;

    *syn = tcp_header->syn;
    *ack = tcp_header->ack;
    *fin = tcp_header->fin;

    uint16_t reference_cksum = tcp_header->check;
    tcp_header->check = 0;

    uint16_t cksum = tcp_checksum((uint8_t*) tcp_header, tcp_segment_size, sip, dip);

    tcp_header->check = reference_cksum;

    printf("Protocol: %u; TCP Seqno: %u; TCP checksum: %hx; computed TCP checksum: %hx\n", (uint32_t)ip_hdr->protocol, NTOHL(tcp_header->seq), reference_cksum, (uint16_t)cksum);

    if (((uint16_t)cksum) != reference_cksum && fix)
    {
        printf("Fixing cksum!\n");
        tcp_header->check = ((uint16_t)cksum);
        return true;
    }

    return (((uint16_t)cksum) == reference_cksum);
}

int pcap_trace_open(const char *pcap_file, pcap_trace_t* trace)
{
    *trace = fopen(pcap_file, "rb");
    if (*trace==NULL)
    {
        printf("Cannot open PCAP file %s\n", pcap_file);
        return -1;
    }

    //read trace header
    pcap_hdr_t pcap_hdr;
    fread(&pcap_hdr, sizeof(pcap_hdr_t), 1, *trace);

    return 0;
}

int pcap_trace_close(pcap_trace_t trace)
{
    return fclose(trace);
}

int pcap_read_packet(pcap_trace_t trace, const char* filter_ip, uint8_t* pkt_data, size_t max_pkt_len, size_t *actual_pkt_len, uint8_t *syn, uint8_t *ack, uint8_t *fin)
{
    pcaprec_hdr_t pkt_hdr;
    size_t read = fread(&pkt_hdr, 1, sizeof(pcaprec_hdr_t), trace);

    while (read > 0)
    {
        assert(pkt_hdr.incl_len == pkt_hdr.orig_len);

        bool forced_offset = 0;

        printf("pkt len: %u\n", pkt_hdr.incl_len);
  
        //remove ethernet address
        ether_header_t eth_hdr;
        fread(&eth_hdr, 1, sizeof(ether_header_t), trace);
        size_t ip_pkt_size = pkt_hdr.incl_len - sizeof(ether_header_t);

        assert(ip_pkt_size <= max_pkt_len);
        size_t read = fread(pkt_data, 1, ip_pkt_size, trace);
        assert(read == pkt_hdr.incl_len - sizeof(ether_header_t));

        ip_header_t *ip_hdr = (ip_header_t *)&(pkt_data[0]);

        if (!tcp_is_valid((uint8_t *)(ip_hdr), 1, syn, ack, fin))
        {
            printf("**** Warning: Checksumm is wrong! Fixing it. ****\n");
        }

        char sip[IP_STR_LEN];
        char dip[IP_STR_LEN];
        int2ip(ip_hdr->saddr, sip, IP_STR_LEN);
        int2ip(ip_hdr->daddr, dip, IP_STR_LEN);

        if (!strcmp(dip, filter_ip))
        {
            *actual_pkt_len = ip_pkt_size;
            printf("genfun: source: %s; dest: %s; len: %u\n", sip, dip, ip_pkt_size);
            return 0;
        }
        else
        {
            read = fread(&pkt_hdr, 1, sizeof(pcaprec_hdr_t), trace);
        }
    }

    return -1;
}


