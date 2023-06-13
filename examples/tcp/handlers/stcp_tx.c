#include <handler.h>

#include "stcp_tx.h"
#include "stcp_utils.h"
#include "ip.h"

void stcp_tx_ack(stcp_connection_t *connection, tcp_header_t *tcp_hdr, uint32_t to_ack)
{
    /* get ip packet */
    ip_header_t *ip_hdr = (ip_header_t*) (((uint8_t*) tcp_hdr) - sizeof(ip_header_t));
    uint32_t ip_source = ip_hdr->saddr;
    ip_hdr->saddr = ip_hdr->daddr;
    ip_hdr->daddr = ip_source;
    uint16_t tcp_source = tcp_hdr->source;
    tcp_hdr->source = tcp_hdr->dest;
    tcp_hdr->dest = tcp_source;
    tcp_hdr->ack = 1;

    spin_cmd_t cmd; /* we don't need to explicitly track its completion */
#ifdef SPIN_SNITCH
    spin_nic_packet_send((void*) ip_hdr, sizeof(ip_header_t) + sizeof(tcp_header_t), &cmd);
#else
    spin_send_packet((void*) ip_hdr, sizeof(ip_header_t) + sizeof(tcp_header_t), &cmd);
#endif
}

void stcp_tx_handle_ack(stcp_connection_t *connection, uint32_t ackdseqno)
{
    uint32_t rackdseqno = NTOHL(ackdseqno) - connection->tx_isn;
    STCP_DPRINTF("got ack for seqno %lu\n", rackdseqno);
}
