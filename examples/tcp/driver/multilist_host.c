#include "multilist_host.h"

#include <assert.h>

int multilist_host_init(multilist_t *mlist, uint32_t capacity, uint32_t num_lists, uint32_t nic_mem_address, uint32_t nic_mem_size)
{    
    assert(num_lists == NUM_LISTS); // for now this is fixed at compile time
    assert(sizeof(PTR32(void*))==4);

    // only power of two (because of easier free pool management)
    assert((capacity & (capacity-1)) == 0);

    mlist->compact_list_head = 0;
    for (int i=0; i<num_lists; i++)
    {
        mlist->list_head[i] = 0;   
        mlist->list_lock[i] = 0;
    }

    uint32_t list_nodes_offset = sizeof(multilist_t);
    uint32_t free_pool_storage_offset = sizeof(multilist_t) + capacity * sizeof(multilist_node_t);

    mlist->num_nodes = capacity;
    mlist->compact_list_head = 0;

    //mlist->nodes = /*nic_mem_address +*/ list_nodes_offset; //TO BE FIXED BY HH

    /*
    mlist->free_pool.size = capacity;
    mlist->free_pool.storage = free_pool_storage_offset; //TO BE FIXED BY HH
    mlist->free_pool.head = 0;
    mlist->free_pool.tail = capacity - 1;
    mlist->free_pool.mask = capacity - 1;
    mlist->free_pool.lock = 0;
    mlist->free_pool.base_addr = 0; // TO BE FIXED BY HH

    printf("Free pool storage offset: %u; sizeof(multilist_t): %lu; capacity: %u; sizeof(multilist_node_t): %lu\n", free_pool_storage_offset, sizeof(multilist_t), capacity, sizeof(multilist_node_t));
    uint32_t *free_pool_storage_ptr = (uint32_t*) (((uint8_t *) mlist) + free_pool_storage_offset);
    for (int i=0; i<capacity; i++)
    {
        // these won't be fixed by the HH but on-demand by the pool manager
        free_pool_storage_ptr[i] = list_nodes_offset + i*sizeof(multilist_node_t);
        //printf("free pool (%lu): %lx\n", ((uint8_t*) &free_pool_storage_ptr[i]) - ((uint8_t*) mlist), free_pool_storage_ptr[i]);
    }
    */

    mlist->free_pool.count = capacity;
    mlist->free_pool.obj_size = sizeof(multilist_node_t);
    mlist->free_pool.mem_base_addr_start = list_nodes_offset;
    mlist->free_pool.mem_base_addr_end = list_nodes_offset + multilist_host_additional_size(capacity, num_lists);
    
    for (int i=0; i<NUM_POOLS; i++) 
    {
        mlist->free_pool.free_stack_top[i] = 0;
        mlist->free_pool.free_stack_lock[i] = 0;
    }

    return 0;
}

uint32_t multilist_host_additional_size(uint32_t capacity, uint32_t num_lists)
{
    assert(num_lists == NUM_LISTS);

    return capacity * (sizeof(multilist_node_t));
}