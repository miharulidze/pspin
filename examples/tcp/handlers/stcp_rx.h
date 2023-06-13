#pragma once

#include <handler.h>

#include <stcp.h>
#include <tcp.h>

void stcp_rx(handler_args_t *handler_args, stcp_connection_t *connection,
             tcp_header_t *tcp_hdr, size_t tcp_seg_size);
