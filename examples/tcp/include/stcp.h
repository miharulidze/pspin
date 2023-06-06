#pragma once

//#define SPIN_SNITCH

#define STCP_CLOSED 0
#define STCP_LISTEN 1
#define STCP_SYN_RCVD 2
#define STCP_SYN_SENT 3
#define STCP_ESTABLISHED 4
#define STCP_FIN_WAIT_1 5
#define STCP_FIN_WAIT_2 6
#define STCP_CLOSING 7
#define STCP_TIME_WAIT 8
#define STCP_CLOSE_WAIT 9
#define STCP_LAST_ACK 10


#include "stcp_conf.h"
#include "ooo.h"

//#include "spin_conf.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* TYPES */
typedef uint32_t stcp_conn_state_t;

/*
typedef struct hpu_state
{
    stcp_range_t range;
} hpu_state_t;
*/

typedef struct stcp_host_buffer
{
    uint64_t host_ptr;
    uint32_t length;
    uint32_t bytes_left;

    // this should be a flag but Snitch does not support misaligned accesses
    // TODO: have a word of flags in stcp_connection_t.
    uint32_t valid;
} __attribute__((__packed__)) stcp_host_buffer_t;

typedef struct stcp_connnection
{
    volatile int32_t refs;
    stcp_host_buffer_t buffer;
    stcp_conn_state_t curr_state;

    //initial seqno
    uint32_t rx_isn;
    uint32_t tx_isn;

    //handling connection termination
    uint32_t fin_seqno;
    uint32_t fin_seen;

    //rcv window pointers
    volatile uint32_t next_to_receive __attribute__ ((aligned (4)));
    volatile uint32_t next_to_read __attribute__ ((aligned (4)));
    volatile uint32_t next_to_ack __attribute__ ((aligned (4)));

    volatile uint32_t ooo_threads __attribute__ ((aligned (4)));
    volatile uint32_t ooo_lock __attribute__ ((aligned (4)));
    volatile uint32_t ooo_dirty __attribute__ ((aligned (4)));

    ooo_manager_t ooo_man;
} __attribute__((__packed__)) stcp_connection_t;


typedef struct cluster_state
{
#ifdef CONNECTION_IN_L1
    stcp_connection_t connection;
#endif
} cluster_state_t;
