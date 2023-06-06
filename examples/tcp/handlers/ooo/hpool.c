#include "hpool.h"
#include <handler.h>

#define HPOOL_MASK 7
#define NUM_OBJECTS_SHARED_ALLOC 4

uint32_t _push(hpool_t *hpool, uint32_t hpool_idx, uint8_t* obj_ptr);
uint32_t _pop(hpool_t *hpool, uint32_t hpool_idx, uint8_t** obj_ptr);

uint32_t _push(hpool_t *hpool, uint32_t hpool_idx, uint8_t* obj_ptr)
{
    hpool_entry_t* entry = (hpool_entry_t*) obj_ptr;
    entry->next = hpool->free_stack_top[hpool_idx];
    hpool->free_stack_top[hpool_idx] = entry;

    return HPOOL_SUCCESS;
}

uint32_t _pop(hpool_t *hpool, uint32_t hpool_idx, uint8_t** obj_ptr)
{
    *obj_ptr = (uint8_t*) hpool->free_stack_top[hpool_idx];
    hpool->free_stack_top[hpool_idx] = hpool->free_stack_top[hpool_idx]->next;
    return HPOOL_SUCCESS;
}

uint32_t hpool_push(hpool_t *hpool, uint32_t hpool_idx, uint8_t* obj_ptr)
{
    spin_lock_lock(&(hpool->free_stack_lock[hpool_idx]));
    uint32_t res = _push(hpool, hpool_idx, obj_ptr);
    spin_lock_unlock(&(hpool->free_stack_lock[hpool_idx]));

    return res;
}

uint32_t hpool_pop(hpool_t *hpool, uint32_t hpool_idx, uint8_t** obj_ptr)
{
    // (1) allocate from my pool
    spin_lock_lock(&(hpool->free_stack_lock[hpool_idx]));
    if (hpool->free_stack_top[hpool_idx] != NULL)
    {
        _pop(hpool, hpool_idx, obj_ptr);
        spin_lock_unlock(&(hpool->free_stack_lock[hpool_idx]));
        return HPOOL_SUCCESS;
    }
    spin_lock_unlock(&(hpool->free_stack_lock[hpool_idx]));

    // (2) allocate from initial shared chunk of memory
    if (hpool->mem_base_addr_start < hpool->mem_base_addr_end)
    {
        uint32_t alloc_base = amo_add(&(hpool->mem_base_addr_start), NUM_OBJECTS_SHARED_ALLOC * hpool->obj_size);
        
        // we need to recheck as there could have been a race and now there could be no space
        if (alloc_base < hpool->mem_base_addr_end)
        {        
            *obj_ptr = (uint8_t *) alloc_base;

            // push the additional ones we got from the shared mem
            spin_lock_lock(&(hpool->free_stack_lock[hpool_idx]));
            for (int i=1; i<NUM_OBJECTS_SHARED_ALLOC; i++)
            {
                alloc_base += hpool->obj_size;
                _push(hpool, hpool_idx, (uint8_t*) alloc_base);
            }
            spin_lock_unlock(&(hpool->free_stack_lock[hpool_idx]));

            return HPOOL_SUCCESS;
        }
    }

    // (3) go steal from somebody else
    for (int i=0; i<NUM_POOLS; i++)
    {
        uint32_t steal_pool_idx = (hpool_idx + i) & HPOOL_MASK;

        // this could be a try_lock and have a more approximate approach (i.e., false negatives)
        spin_lock_lock(&(hpool->free_stack_lock[steal_pool_idx]));
        if (hpool->free_stack_top[steal_pool_idx] != NULL)
        {
            _pop(hpool, steal_pool_idx, obj_ptr);
            spin_lock_unlock(&(hpool->free_stack_lock[steal_pool_idx]));
            return HPOOL_SUCCESS;
        }

        spin_lock_unlock(&(hpool->free_stack_lock[steal_pool_idx]));
    }

    return HPOOL_NO_SPACE;
}
