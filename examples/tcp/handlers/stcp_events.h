#pragma once

#include <stcp.h>
#include <tcp.h>

/** EVENTS **/
typedef uint16_t stcp_event_t;
#define STCP_EVT_ERR_PROTO 0
#define STCP_EVT_FIN 1
#define STCP_EVT_CLOSE 2
#define STCP_EVT_DATA 3

void stcp_evt_error(stcp_connection_t *connection, tcp_header_t *tcp_hdr, stcp_event_t error);
void stcp_evt(stcp_connection_t *connection, tcp_header_t *tcp_hdr, stcp_event_t evt);
