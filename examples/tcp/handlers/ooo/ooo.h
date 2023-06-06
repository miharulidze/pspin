#pragma once

#include <stdint.h>

#define OOO_SUCCESS 0
#define OOO_NO_SPACE 1
#define OOO_FLUSHED 2

#ifdef OOO_LAZYLIST
#include "lazylist.h"
typedef lazylist_t ooo_manager_t;
typedef lazylist_range_t ooo_range_t;
#else
#include "multilist.h"
typedef multilist_t ooo_manager_t;
typedef multilist_range_t ooo_range_t;
#endif

uint32_t ooo_range_push(ooo_manager_t *ooo_man, uint32_t hpu_id, uint32_t cluster_id, ooo_range_t range);
uint32_t ooo_range_front_flush(ooo_manager_t *ooo_man, uint32_t hpu_id, uint32_t cluster_id, int32_t left);
uint32_t ooo_range_front_force_flush(ooo_manager_t *ooo_man, uint32_t hpu_id, uint32_t cluster_id, int32_t left);
