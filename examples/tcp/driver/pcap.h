#pragma once

#include <stdio.h>
#include <stdint.h>

#define guint32 uint32_t
#define guint16 uint16_t
#define gint32 int32_t

#define IP_STR_LEN 1024

typedef FILE* pcap_trace_t;

typedef struct pcap_hdr_s
{
    guint32 magic_number;  /* magic number */
    guint16 version_major; /* major version number */
    guint16 version_minor; /* minor version number */
    gint32 thiszone;       /* GMT to local correction */
    guint32 sigfigs;       /* accuracy of timestamps */
    guint32 snaplen;       /* max length of captured packets, in octets */
    guint32 network;       /* data link type */
} __attribute__((packed)) pcap_hdr_t;

typedef struct pcaprec_hdr_s
{
    guint32 ts_sec;   /* timestamp seconds */
    guint32 ts_usec;  /* timestamp microseconds */
    guint32 incl_len; /* number of octets of packet saved in file */
    guint32 orig_len; /* actual length of packet */
} __attribute__((packed)) pcaprec_hdr_t;

int pcap_trace_open(const char *pcap_file, pcap_trace_t* trace);
int pcap_trace_close(pcap_trace_t trace);
int pcap_read_packet(pcap_trace_t trace, const char* filter_ip, uint8_t* pkt_data, size_t max_pkt_len, size_t *actual_pkt_len, uint8_t *syn, uint8_t *ack, uint8_t *fin);

//this has nothing to do with PCAP, but it's useful
void int2ip(uint32_t ip, char *buff, size_t len);

uint16_t tcp_checksum(uint8_t* tcp_segment, uint32_t tcp_segment_size, uint32_t ip_src, uint32_t ip_dst);