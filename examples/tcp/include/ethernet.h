#pragma once
#include <stdint.h>

#define ETH_ALEN 6

/* 10Mb/s ethernet header */
typedef struct ether_header
{
    uint8_t ether_dhost[ETH_ALEN]; /* destination eth addr        */
    uint8_t ether_shost[ETH_ALEN]; /* source ether addr        */
    uint16_t ether_type;           /* packet type ID field        */
} __attribute__((__packed__)) ether_header_t;

/* Ethernet protocol ID's */
#define ETHERTYPE_PUP 0x0200      /* Xerox PUP */
#define ETHERTYPE_SPRITE 0x0500   /* Sprite */
#define ETHERTYPE_IP 0x0800       /* IP */
#define ETHERTYPE_ARP 0x0806      /* Address resolution */
#define ETHERTYPE_REVARP 0x8035   /* Reverse ARP */
#define ETHERTYPE_AT 0x809B       /* AppleTalk protocol */
#define ETHERTYPE_AARP 0x80F3     /* AppleTalk ARP */
#define ETHERTYPE_VLAN 0x8100     /* IEEE 802.1Q VLAN tagging */
#define ETHERTYPE_IPX 0x8137      /* IPX */
#define ETHERTYPE_IPV6 0x86dd     /* IP protocol version 6 */
#define ETHERTYPE_LOOPBACK 0x9000 /* used to test interfaces */

#define ETHER_ADDR_LEN ETH_ALEN                       /* size of ethernet addr */
#define ETHER_TYPE_LEN 2                              /* bytes in type field */
#define ETHER_CRC_LEN 4                               /* bytes in CRC field */
#define ETHER_HDR_LEN ETH_HLEN                        /* total octets in header */
#define ETHER_MIN_LEN (ETH_ZLEN + ETHER_CRC_LEN)      /* min packet length */
#define ETHER_MAX_LEN (ETH_FRAME_LEN + ETHER_CRC_LEN) /* max packet length */

/* make sure ethenet length is valid */
#define ETHER_IS_VALID_LEN(foo) \
    ((foo) >= ETHER_MIN_LEN && (foo) <= ETHER_MAX_LEN)

/*
 * The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
 * (type-ETHERTYPE_TRAIL)*512 bytes of data followed
 * by an ETHER type (as given above) and then the (variable-length) header.
 */
#define ETHERTYPE_TRAIL 0x1000 /* Trailer packet */
#define ETHERTYPE_NTRAILER 16

#define ETHERMTU ETH_DATA_LEN
#define ETHERMIN (ETHER_MIN_LEN - ETHER_HDR_LEN - ETHER_CRC_LEN)
