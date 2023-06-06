#pragma once

#include "hpool.h"
#include "../stcp_utils.h"

#define NUM_LISTS 8

typedef struct multilist_range
{
    uint32_t left;
    uint32_t right;
} multilist_range_t;

typedef struct multilist_node
{
    multilist_range_t               range;
    PTR32(struct multilist_node*)   next;
} __attribute__((__packed__)) multilist_node_t;

typedef struct multilist
{
    PTR32(multilist_node_t*)    compact_list_head;
    PTR32(multilist_node_t*)    list_head[NUM_LISTS];
    volatile uint32_t           list_lock[NUM_LISTS]  __attribute__ ((aligned (4)));
    uint32_t                    num_nodes;
    //PTR32(multilist_node_t*)    nodes;
    hpool_t                     free_pool;
} __attribute__((__packed__)) multilist_t;


uint32_t multilist_range_push(multilist_t *ooo_man, uint32_t list_id, multilist_range_t range);
//uint32_t multilist_front_flush_if(multilist_t *ooo_man, uint32_t list_id, uint32_t left, uint32_t *right);
uint32_t multilist_front_flush(multilist_t *mlist, uint32_t list_id, uint32_t left, uint32_t max_iters);