#pragma once
/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions for the TCP protocol.
 *
 * Version:	@(#)tcp.h	1.0.2	04/28/93
 *
 * Author:	Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */

#include "types.h"

// Using little endian

typedef struct tcphdr
{
    __be16 source;
    __be16 dest;
    __be32 seq;
    __be32 ack_seq;
    __u16 res1 : 4,
        doff : 4,
        fin : 1,
        syn : 1,
        rst : 1,
        psh : 1,
        ack : 1,
        urg : 1,
        ece : 1,
        cwr : 1;
    __be16 window;
    __sum16 check;
    __be16 urg_ptr;
}  __attribute__((__packed__)) tcp_header_t;

/*
 *	The union cast uses a gcc extension to avoid aliasing problems
 *  (union is compatible to any of its members)
 *  This means this part of the code is -fstrict-aliasing safe now.
 */
union tcp_word_hdr {
    struct tcphdr hdr;
    __be32 words[5];
};

#define tcp_flag_word(tp) (((union tcp_word_hdr *)(tp))->words[3])

enum
{
    TCP_FLAG_CWR = __constant_cpu_to_be32(0x00800000),
    TCP_FLAG_ECE = __constant_cpu_to_be32(0x00400000),
    TCP_FLAG_URG = __constant_cpu_to_be32(0x00200000),
    TCP_FLAG_ACK = __constant_cpu_to_be32(0x00100000),
    TCP_FLAG_PSH = __constant_cpu_to_be32(0x00080000),
    TCP_FLAG_RST = __constant_cpu_to_be32(0x00040000),
    TCP_FLAG_SYN = __constant_cpu_to_be32(0x00020000),
    TCP_FLAG_FIN = __constant_cpu_to_be32(0x00010000),
    TCP_RESERVED_BITS = __constant_cpu_to_be32(0x0F000000),
    TCP_DATA_OFFSET = __constant_cpu_to_be32(0xF0000000)
};

/*
 * TCP general constants
 */
#define TCP_MSS_DEFAULT 536U  /* IPv4 (RFC1122, RFC2581) */
#define TCP_MSS_DESIRED 1220U /* IPv6 (tunneled), EDNS0 (RFC3226) */

/* TCP socket options */
#define TCP_NODELAY 1               /* Turn off Nagle's algorithm. */
#define TCP_MAXSEG 2                /* Limit MSS */
#define TCP_CORK 3                  /* Never send partially complete segments */
#define TCP_KEEPIDLE 4              /* Start keeplives after this period */
#define TCP_KEEPINTVL 5             /* Interval between keepalives */
#define TCP_KEEPCNT 6               /* Number of keepalives before death */
#define TCP_SYNCNT 7                /* Number of SYN retransmits */
#define TCP_LINGER2 8               /* Life time of orphaned FIN-WAIT-2 state */
#define TCP_DEFER_ACCEPT 9          /* Wake up listener only when data arrive */
#define TCP_WINDOW_CLAMP 10         /* Bound advertised window */
#define TCP_INFO 11                 /* Information about this connection. */
#define TCP_QUICKACK 12             /* Block/reenable quick acks */
#define TCP_CONGESTION 13           /* Congestion control algorithm */
#define TCP_MD5SIG 14               /* TCP MD5 Signature (RFC2385) */
#define TCP_THIN_LINEAR_TIMEOUTS 16 /* Use linear timeouts for thin streams*/
#define TCP_THIN_DUPACK 17          /* Fast retrans. after 1 dupack */
#define TCP_USER_TIMEOUT 18         /* How long for loss retry before timeout */
#define TCP_REPAIR 19               /* TCP sock is under repair right now */
#define TCP_REPAIR_QUEUE 20
#define TCP_QUEUE_SEQ 21
#define TCP_REPAIR_OPTIONS 22
#define TCP_FASTOPEN 23 /* Enable FastOpen on listeners */
#define TCP_TIMESTAMP 24
#define TCP_NOTSENT_LOWAT 25      /* limit number of unsent bytes in write queue */
#define TCP_CC_INFO 26            /* Get Congestion Control (optional) info */
#define TCP_SAVE_SYN 27           /* Record SYN headers for new connections */
#define TCP_SAVED_SYN 28          /* Get SYN headers recorded for connection */
#define TCP_REPAIR_WINDOW 29      /* Get/set window parameters */
#define TCP_FASTOPEN_CONNECT 30   /* Attempt FastOpen with connect */
#define TCP_ULP 31                /* Attach a ULP to a TCP connection */
#define TCP_MD5SIG_EXT 32         /* TCP MD5 Signature with extensions */
#define TCP_FASTOPEN_KEY 33       /* Set the key for Fast Open (cookie) */
#define TCP_FASTOPEN_NO_COOKIE 34 /* Enable TFO without a TFO cookie */
#define TCP_ZEROCOPY_RECEIVE 35
#define TCP_INQ 36 /* Notify bytes available to read as a cmsg on read */

#define TCP_CM_INQ TCP_INQ

#define TCP_TX_DELAY 37 /* delay outgoing packets by XX usec */

#define TCP_REPAIR_ON 1
#define TCP_REPAIR_OFF 0
#define TCP_REPAIR_OFF_NO_WP -1 /* Turn off without window probes */

