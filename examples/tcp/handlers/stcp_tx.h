#pragma once

#include <stcp.h>
#include <tcp.h>

void stcp_tx_ack(stcp_connection_t *connection, tcp_header_t *tcp_hdr, uint32_t to_ack);
void stcp_tx_handle_ack(stcp_connection_t *connection, uint32_t ackdseqno);
