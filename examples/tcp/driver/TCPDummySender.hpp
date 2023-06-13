#pragma once

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <algorithm>

#include <tcp.h>
#include <ip.h>
#include <ethernet.h>
#include <types.h>

#include "TCPSender.hpp"

namespace STCP
{
    class TCPDummySender : public TCPSender
    {
    private:
        typedef enum
        {
            INIT = 0,
            SYN_SENT,
            SYN_ACK_RECVD,
            ESTABLISHED,
            FIN_SENT,
            FIN_ACK_RECVD,
            CLOSED
        } state_t;

        typedef struct tcp_flow
        {
            uint32_t next_seqno;
            state_t state;
        } tcp_flow_t;

    private:
        tcp_flow_t tcp_flow;
        size_t bytes_left;
        size_t bytes_sent;
        size_t MTU;

        uint32_t ipsrc, ipdst;

    private:
        size_t _make_pkt(uint8_t *pkt_buffer, size_t pkt_buffer_size,
                         bool syn, bool ack, bool fin, size_t payload_size)
        {
            size_t packet_len = sizeof(ip_header_t) + sizeof(tcp_header_t) + payload_size;
            ip_header_t *ip_hdr = (ip_header_t *)(&(pkt_buffer[0]));
            tcp_header_t *tcp_hdr = (tcp_header_t *)(&(pkt_buffer[0]) + sizeof(ip_header_t));

            assert(packet_len <= pkt_buffer_size);

            // define IP header
            ip_hdr->ihl = 5;
            ip_hdr->version = 4;
            ip_hdr->tos = 0;
            ip_hdr->tot_len = HTONS(packet_len);
            ip_hdr->id = 0;
            ip_hdr->frag_off = 0;
            ip_hdr->ttl = 0;
            ip_hdr->protocol = IPPROTO_TCP;
            ip_hdr->check = 0;
            ip_hdr->saddr = ipsrc;
            ip_hdr->daddr = ipdst;

            tcp_hdr->source = 4040;
            tcp_hdr->dest = 8080;
            tcp_hdr->seq = HTONL(tcp_flow.next_seqno);
            tcp_hdr->ack_seq = 0;

            tcp_hdr->doff = 5;
            tcp_hdr->res1 = 0;
            tcp_hdr->cwr = 0;
            tcp_hdr->ece = 0;
            tcp_hdr->urg = 0;
            tcp_hdr->ack = (ack) ? 1 : 0;
            tcp_hdr->psh = 0;
            tcp_hdr->rst = 0;
            tcp_hdr->syn = (syn) ? 1 : 0;
            tcp_hdr->fin = (fin) ? 1 : 0;
            tcp_hdr->window = 1024;
            tcp_hdr->check = 0; // will compute below
            tcp_hdr->urg_ptr = 0;

            tcp_flow.next_seqno += payload_size;

            tcp_hdr->check = computeChecksum((uint8_t *)tcp_hdr, packet_len - sizeof(ip_header_t), ipsrc, ipdst);

            return packet_len;
        }

    public:
        TCPDummySender(size_t bytes, size_t MTU, uint32_t ipsrc, uint32_t ipdst, uint32_t start_seqno)
            : bytes_left(bytes), bytes_sent(0), MTU(MTU), ipsrc(ipsrc), ipdst(ipdst)
        {
            tcp_flow.state = INIT;
            tcp_flow.next_seqno = HTONL(start_seqno);
        }

        bool isReady()
        {
            return tcp_flow.state == INIT || tcp_flow.state == FIN_ACK_RECVD || tcp_flow.state == ESTABLISHED;
        }

        size_t getNextPacket(Packet* pkt) override
        {
            pkt->eos = false;

            if (tcp_flow.state == INIT) {
                printf("[HOST] sending SYN!\n");
                pkt->size = _make_pkt(&(pkt->data[0]), pkt->mtu, 1, 0, 0, 0);
                tcp_flow.state = SYN_SENT;
                return pkt->size;
            } else if (tcp_flow.state == FIN_ACK_RECVD) {
                printf("[HOST] sending final ACK!\n");
                pkt->size = _make_pkt(&(pkt->data[0]), pkt->mtu, 0, 1, 0, 0);
                pkt->eos = true;
                tcp_flow.state = CLOSED;
                return pkt->size;
            } else  { // make full packet
                assert(bytes_left > 0);
                bool fin = false;
                bool ack = bytes_sent == 0; // this is for the final ACK of the three-way handshake
                size_t max_payload_size = MTU - sizeof(ip_header_t) - sizeof(tcp_header_t);
                size_t payload_size = std::min(max_payload_size, bytes_left);

                bytes_left -= payload_size;
                bytes_sent += payload_size;

                if (bytes_left == 0) {
                    tcp_flow.state = FIN_SENT;
                    fin = true;
                }

                pkt->size = _make_pkt(&(pkt->data[0]), pkt->mtu, 0, ack, bytes_left == 0, payload_size);
                ip_header_t *ip_hdr = (ip_header_t *)(&(pkt->data[0]));
                tcp_header_t *tcp_hdr = (tcp_header_t *)(&(pkt->data[0]) + sizeof(ip_header_t));

                printf("Sending TCP packet (len: %lu; SYN: %hu; ACK: %hu; FIN: %hu; CHECK: 0x%x); bytes_left: %u;\n",
                       pkt->size, (uint16_t)tcp_hdr->syn, (uint16_t)tcp_hdr->ack, (uint16_t)tcp_hdr->fin,
                       (uint32_t)tcp_hdr->check, bytes_left);

                return pkt->size;
            }
        }

        void notifyPacket(uint8_t *data, size_t len)
        {
            char saddr[IP_STR_LEN];
            char daddr[IP_STR_LEN];
            ip_header_t *ip_hdr = (ip_header_t *)data;
            int2ip(ip_hdr->saddr, saddr, IP_STR_LEN);
            int2ip(ip_hdr->daddr, daddr, IP_STR_LEN);
            tcp_header_t *tcp_hdr = (tcp_header_t *)(data + sizeof(ip_header_t));

            printf("[HOST] Got packet: %s:%hu -> %s:%hu; SYN: %hu; ACK: %hu; FIN: %hu; State: %u\n",
                   saddr, tcp_hdr->source, daddr, tcp_hdr->dest,
                   (uint16_t)tcp_hdr->syn, (uint16_t)tcp_hdr->ack, (uint16_t)tcp_hdr->fin,
                   (uint32_t)tcp_flow.state);

            switch (tcp_flow.state) {

            case SYN_SENT:
                assert(tcp_hdr->syn && tcp_hdr->ack);
                tcp_flow.state = ESTABLISHED;
                break;

            case ESTABLISHED:
                break;

            case FIN_SENT:
                if (tcp_hdr->fin && tcp_hdr->ack)
                    tcp_flow.state = FIN_ACK_RECVD;
                break;
            default:
                //assert(0);
                break;
            }
        }

        size_t bytesSent()
        {
            return bytes_sent;
        }
    };
}
