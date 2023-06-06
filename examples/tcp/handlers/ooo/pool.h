#pragma once

#include <stdint.h>
#include "../stcp_utils.h"

#define POOL_SUCCESS 0
#define POOL_NO_SPACE 1

typedef struct pool
{
    uint32_t            size;
    PTR32(uint32_t*)    storage; //these are offsets
    uint32_t            head, tail, mask;
    PTR32(uint8_t*)     base_addr;
    volatile uint32_t   lock __attribute__ ((aligned(4)));
} pool_t;

uint32_t pool_push(pool_t *pool, uint8_t* pool_obj_ptr);
uint32_t pool_pop(pool_t *pool, uint8_t** pool_obj_ptr);
uint32_t pool_is_empty(pool_t *pool);
uint32_t pool_is_full(pool_t *pool);
