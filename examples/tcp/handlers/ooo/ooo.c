#include "ooo.h"

uint32_t ooo_range_front_flush(ooo_manager_t *ooo_man, uint32_t hpu_id, uint32_t cluster_id, int32_t left)
{
#ifdef OOO_LAZYLIST

#else
    return multilist_front_flush((multilist_t*) ooo_man, hpu_id, left, 1);
#endif
}

uint32_t ooo_range_front_force_flush(ooo_manager_t *ooo_man, uint32_t hpu_id, uint32_t cluster_id, int32_t left)
{
#ifdef OOO_LAZYLIST

#else
    return multilist_front_flush((multilist_t*) ooo_man, hpu_id, left, -1);
#endif
}

uint32_t ooo_range_push(ooo_manager_t *ooo_man, uint32_t hpu_id, uint32_t cluster_id, ooo_range_t range)
{
#ifdef OOO_LAZYLIST

#else
    return multilist_range_push((multilist_t *) ooo_man, hpu_id, range);
#endif
}
