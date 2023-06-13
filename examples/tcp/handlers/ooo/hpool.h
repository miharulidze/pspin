#pragma once

#include <stdint.h>
#include "types.h"

#define NUM_POOLS 8
#define HPOOL_SUCCESS 0
#define HPOOL_NO_SPACE 1

typedef struct hpool_entry
{
    struct hpool_entry* next;
} __attribute__((__packed__, aligned(4))) hpool_entry_t;

typedef struct hpool
{
    uint32_t                        count;
    uint32_t                        obj_size;
    uint32_t                        mem_base_addr_start __attribute__ ((aligned(4)));
    uint32_t                        mem_base_addr_end;
    PTR32(hpool_entry_t*)           free_stack_top[NUM_POOLS];
    volatile uint32_t               free_stack_lock[NUM_POOLS] __attribute__ ((aligned(4)));
} __attribute__((__packed__, aligned(4))) hpool_t;

uint32_t hpool_push(hpool_t *hpool, uint32_t hpool_idx, uint8_t* obj_ptr);
uint32_t hpool_pop(hpool_t *hpool, uint32_t hpool_idx, uint8_t** obj_ptr);
