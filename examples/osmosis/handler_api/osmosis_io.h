#include <handler.h>

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
osmosis_dma(void* source, void* dest, size_t length,
            int direction, int options, osmosis_dma_t* xfer,
            uint32_t chunk_size)
{
    spin_dma_t tmp;

    /* Batch tail */
    for (; length >= chunk_size; length -= chunk_size) {
        spin_dma(source, dest, chunk_size, direction, options, &tmp);
        source = (char *)source + chunk_size;
        dest = (char *)dest + chunk_size;
	spin_dma_wait(tmp);
    }

    /* And the last one... */
    if (length > 0) {
        spin_dma(source, dest, length, direction, options, &tmp);
        spin_dma_wait(tmp);
    }

    return SPIN_OK;
}

static inline int
osmosis_dma_to_host(uint64_t host_addr, uint32_t nic_addr,
                    uint32_t length, bool generate_event,
                    osmosis_cmd_t *xfer,
                    uint32_t chunk_size)
{
    spin_cmd_t tmp;

    for (; length >= chunk_size; length -= chunk_size) {
        spin_dma_to_host(host_addr, nic_addr, chunk_size, generate_event, &tmp);
        host_addr = host_addr + chunk_size;
        nic_addr = nic_addr + chunk_size;
        spin_cmd_wait(tmp);
    }

    if (length > 0) {
        spin_dma_to_host(host_addr, nic_addr, length, generate_event, &tmp);
        spin_cmd_wait(tmp);
    }

    return SPIN_OK;
}

static inline int
osmosis_dma_from_host(uint64_t host_addr, uint32_t nic_addr,
                      uint32_t length, bool generate_event,
                      osmosis_cmd_t *xfer,
                      uint32_t chunk_size)
{
    spin_cmd_t tmp;

    for (; length >= chunk_size; length -= chunk_size) {
        spin_dma_from_host(host_addr, nic_addr, chunk_size, generate_event, &tmp);
        host_addr = host_addr + chunk_size;
        nic_addr = nic_addr + chunk_size;
        spin_cmd_wait(tmp);
    }

    if (length > 0) {
        spin_dma_from_host(host_addr, nic_addr, length, generate_event, &tmp);
        spin_cmd_wait(tmp);
    }

    return SPIN_OK;
}

static inline int
osmosis_send_packet(void *data, uint32_t length, osmosis_cmd_t *handle,
                    uint32_t chunk_size)
{
    spin_cmd_t xfer;

    for (; length >= chunk_size; length -= chunk_size) {
        spin_send_packet(data, chunk_size, &xfer);
        data = (char *)data + chunk_size;
        spin_cmd_wait(xfer);
    }

    if (length > 0) {
        spin_send_packet(data, length, &xfer);
        spin_cmd_wait(xfer);
    }

    return SPIN_OK;
}
