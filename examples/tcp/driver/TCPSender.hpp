#pragma once
#include <stdint.h>
#include <stdio.h>

#include <ip.h>
#include <types.h>
#include "stcp_host.h"

#define IP_STR_LEN 1024

namespace STCP
{
    class TCPSender
    {
    public:
        virtual size_t getNextPacket(Packet* pkt) = 0;

    protected:
        uint16_t computeChecksum(uint8_t *tcp_segment, size_t tcp_segment_size, uint32_t ip_src, uint32_t ip_dst)
        {
            uint32_t cksum = 0;
            // endianness?
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

            // while (cksum >> 16) { cksum = (cksum & 0xFFFF) + (cksum >> 16); }
            cksum = (cksum >> 16) + (cksum & 0xffff);
            cksum += (cksum >> 16);
            cksum = ~cksum;

            return (uint16_t)cksum;
        }

        void int2ip(uint32_t ip, char *buff, size_t len)
        {
            unsigned char bytes[4];
            bytes[0] = ip & 0xFF;
            bytes[1] = (ip >> 8) & 0xFF;
            bytes[2] = (ip >> 16) & 0xFF;
            bytes[3] = (ip >> 24) & 0xFF;
            snprintf(buff, len, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
        }
    };
}