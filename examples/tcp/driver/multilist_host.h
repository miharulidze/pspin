#pragma once 

#include <handlers/ooo/multilist.h>
#include <stdint.h>

int multilist_host_init(multilist_t *mlist, uint32_t capacity, uint32_t num_lists, uint32_t nic_mem_address, uint32_t nic_mem_size);
uint32_t multilist_host_additional_size(uint32_t capacity, uint32_t num_lists);
