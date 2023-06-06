#include "stcp_events.h"

#include "stcp_utils.h"


void stcp_evt_error(stcp_connection_t *connection, tcp_header_t *tcp_hdr, stcp_event_t error){
    //TODO: this should signal the error to the host
    STCP_DPRINTF("ERROR: %lu\n", (uint32_t) error);
}

void stcp_evt(stcp_connection_t *connection, tcp_header_t *tcp_hdr, stcp_event_t evt){
    STCP_DPRINTF("STCP EVENT: %lu!\n", (uint32_t) evt);
}
