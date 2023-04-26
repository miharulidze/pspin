#include <handler.h>

#ifdef OSMOSIS_IO_MAX_INFLIGHT
#define MAX_INFLIGHT_DMAS OSMOSIS_IO_MAX_INFLIGHT
#else
#define MAX_INFLIGHT_DMAS 1
#endif

typedef spin_dma_t osmosis_dma_t;

static inline int
osmosis_dma_wait(osmosis_dma_t xfer)
{
    /* nothing to do */
}

typedef spin_cmd_t osmosis_cmd_t;

static inline int
osmosis_cmd_wait(osmosis_cmd_t handle)
{
    /* nothing to do */
}

static inline int
osmosis_dma(void* source, void* dest, size_t size,
            int direction, int options, osmosis_dma_t* xfer,
            uint32_t chunk_size)
{
    spin_dma_t xfers[MAX_INFLIGHT_DMAS];
    size_t batch_size = chunk_size * MAX_INFLIGHT_DMAS;
    size_t i, j;

    /* Batch chunks */
    for (; size >= batch_size; size -= batch_size) {
        for (i = 0; i < MAX_INFLIGHT_DMAS; i++) {
            spin_dma(source, dest, chunk_size, direction, options, &xfers[i]);
            source = (char *)source + chunk_size;
            dest = (char *)dest + chunk_size;
        }
        for (i = 0; i < MAX_INFLIGHT_DMAS; i++) {
            spin_dma_wait(xfers[i]);
        }
    }

    /* Batch tail */
    for (i = 0; size > chunk_size; size -= chunk_size, i++) {
        spin_dma(source, dest, chunk_size, direction, options, &xfers[i]);
        source = (char *)source + chunk_size;
        dest = (char *)dest + chunk_size;
    }

    /* And the last one... */
    if (size > 0)
        spin_dma(source, dest, size, direction, options, &xfers[i++]);

    for (j = 0; j < i + 1; j++) {
        spin_dma_wait(spin_dma_wait(xfers[j]));
    }

    return SPIN_OK;
}

static inline int
osmosis_dma_to_host(uint64_t host_addr, uint32_t nic_addr,
                    uint32_t length, bool generate_event,
                    osmosis_cmd_t *xfer,
                    uint32_t chunk_size)
{
    spin_cmd_t xfers[MAX_INFLIGHT_DMAS];
    size_t batch_size = chunk_size * MAX_INFLIGHT_DMAS;
    size_t i, j;

    for (; length >= batch_size; length -= batch_size) {
        for (i = 0; i < MAX_INFLIGHT_DMAS; i++) {
            spin_dma_to_host(host_addr, nic_addr, chunk_size, generate_event, &xfers[i]);
            host_addr = host_addr + chunk_size;
            nic_addr = nic_addr + chunk_size;
        }
        for (i = 0; i < MAX_INFLIGHT_DMAS; i++) {
            spin_dma_wait(xfers[i]);
        }
    }

    for (i = 0; length > chunk_size; length -= chunk_size, i++) {
        spin_dma_to_host(host_addr, nic_addr, chunk_size, generate_event, &xfers[i]);
        host_addr = host_addr + chunk_size;
        nic_addr = nic_addr + chunk_size;
    }

    if (length > 0) {
        spin_dma_to_host(host_addr, nic_addr, length, generate_event, &xfers[i++]);
    }

    for (j = 0; j < i + 1; j++) {
        spin_cmd_wait(spin_dma_wait(xfers[j]));
    }

    return SPIN_OK;
}

static inline int
osmosis_dma_from_host(uint64_t host_addr, uint32_t nic_addr,
                      uint32_t length, bool generate_event,
                      osmosis_cmd_t *xfer,
                      uint32_t chunk_size)
{
    spin_cmd_t xfers[MAX_INFLIGHT_DMAS];
    size_t batch_size = chunk_size * MAX_INFLIGHT_DMAS;
    size_t i, j;

    for (; length >= batch_size; length -= batch_size) {
        for (i = 0; i < MAX_INFLIGHT_DMAS; i++) {
            spin_dma_from_host(host_addr, nic_addr, chunk_size, generate_event, &xfers[i]);
            host_addr = host_addr + chunk_size;
            nic_addr = nic_addr + chunk_size;
        }
        for (i = 0; i < MAX_INFLIGHT_DMAS; i++) {
            spin_dma_wait(xfers[i]);
        }
    }

    for (i = 0; length >= chunk_size; length -= chunk_size, i++) {
        spin_dma_from_host(host_addr, nic_addr, chunk_size, generate_event, &xfers[i]);
        host_addr = host_addr + chunk_size;
        nic_addr = nic_addr + chunk_size;
    }

    if (length > 0)
        spin_dma_from_host(host_addr, nic_addr, length, generate_event, &xfers[i++]);

    for (j = 0; j < i + 1; j++) {
        spin_cmd_wait(spin_dma_wait(xfers[j]));
    }

    return SPIN_OK;
}

static inline int
osmosis_send_packet(void *data, uint32_t length, osmosis_cmd_t *handle,
                    uint32_t chunk_size)
{
    spin_cmd_t xfers[MAX_INFLIGHT_DMAS];
    size_t batch_size = chunk_size * MAX_INFLIGHT_DMAS;
    size_t i, j;

    for (; length >= batch_size; length -= batch_size) {
        for (i = 0; i < MAX_INFLIGHT_DMAS; i++) {
            spin_send_packet(data, chunk_size, &xfers[i]);
            data = (char *)data + chunk_size;
        }
        for (i = 0; i < MAX_INFLIGHT_DMAS; i++) {
            spin_dma_wait(xfers[i]);
        }
    }

    for (i = 0; length > chunk_size; length -= chunk_size, i++) {
        spin_send_packet(data, chunk_size, &xfers[i]);
        data = (char *)data + chunk_size;
    }

    if (length > 0)
        spin_send_packet(data, length, &xfers[i++]);

    for (j = 0; j < i + 1; j++) {
        spin_cmd_wait(spin_dma_wait(xfers[j]));
    }

    return SPIN_OK;
}
