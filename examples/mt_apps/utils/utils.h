#include <string.h>

int match_ectx_cb(void *src, void *dst, void *ectx_addr)
{
    if (!strcmp((char *)dst, (char*)ectx_addr))
        return 1;
    return 0;
}
