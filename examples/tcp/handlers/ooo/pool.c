#include "pool.h"

#include <handler.h>
#include "../stcp_utils.h"

uint32_t pool_push(pool_t *pool, uint8_t* pool_obj_ptr)
{
    spin_lock_lock(&pool->lock);

    S_ASSERT(((pool->tail + 1) & pool->mask) != pool->head);

    pool->storage[pool->tail] = pool_obj_ptr - pool->base_addr;
    pool->tail = (pool->tail + 1) & pool->mask;

    spin_lock_unlock(&pool->lock);

    return POOL_SUCCESS;
}

uint32_t pool_pop(pool_t *pool, uint8_t** pool_obj_ptr)
{
    spin_lock_lock(&pool->lock);

    if (pool_is_empty(pool)) 
    {
        spin_lock_unlock(&pool->lock);
        return POOL_NO_SPACE;
    }

    *pool_obj_ptr = pool->base_addr + (uint32_t) pool->storage[pool->head];
    pool->head = (pool->head + 1) & pool->mask;

    spin_lock_unlock(&pool->lock);

    return POOL_SUCCESS;
}

uint32_t pool_is_empty(pool_t *pool)
{
    return pool->head == pool->tail;
}

uint32_t pool_is_full(pool_t *pool)
{
    return (pool->tail & pool->mask) == pool->head;
}

/*
push(pool_t *pool, uint32_t id, uint32_t obj):
    - lock pool(i)
    - push in pool(i)
    - unlock pool(i)

pop(pool_t *pool, uint32_t id, uint32_t *obj):
    - if pool(i) empty: get_some_objects(pool, id);
    
    - lock pool(i)
    - if pool(i) empty: 
        - unlock pool(i)
        - NO SPACE
    - pop element in obj
    - unlock pool(i)

get_some_objects(pool_t *pool, uint32_t id):


*/