#include <handler.h>

#define DMA_CHUNK_SIZE 64
#define MAX_INFLIGHT_DMAS 16

typedef spin_dma_t osmosis_dma_t;

static inline int
osmosis_dma(void* source, void* dest, size_t size,
	    int direction, int options, osmosis_dma_t* xfer)
{
    spin_dma_t xfers[MAX_INFLIGHT_DMAS];
    size_t i, j;

    for (; size > DMA_CHUNK_SIZE; size -= DMA_CHUNK_SIZE * MAX_INFLIGHT_DMAS) {
	for (i = 0; i < MAX_INFLIGHT_DMAS; i++) {
	    spin_dma(source, dest, DMA_CHUNK_SIZE, direction, options, &xfers[i]);
	    source = (char *)source + DMA_CHUNK_SIZE;
	    dest = (char *)dest + DMA_CHUNK_SIZE;
	}
	for (i = 0; i < MAX_INFLIGHT_DMAS; i++) {
	    spin_dma_wait(xfers[i]);
	}
    }

    for (i = 0; size > 0; size -= DMA_CHUNK_SIZE) {
	spin_dma(source, dest, DMA_CHUNK_SIZE, direction, options, &xfers[i]);
	source = (char *)source + DMA_CHUNK_SIZE;
	dest = (char *)dest + DMA_CHUNK_SIZE;
    }

    for (j = 0; j < i + 1; j++) {
	spin_dma_wait(spin_dma_wait(xfers[j]));
    }
}

static inline int
osmosis_dma_wait(osmosis_dma_t xfer)
{
}

typedef spin_cmd_t osmosis_cmd_t;

static inline int
osmosis_cmd_wait(osmosis_cmd_t handle)
{
    return spin_cmd_wait(handle);
}

static inline int
osmosis_send_packet(void *data, uint32_t length, osmosis_cmd_t *handle)
{
    return spin_send_packet(data, length, handle);
}

static inline int
osmosis_dma_to_host(uint64_t host_addr, uint32_t nic_addr,
		    uint32_t length, bool generate_event,
		    osmosis_cmd_t *xfer)
{
    return spin_dma_to_host(host_addr, nic_addr, length, generate_event, xfer);
}

static inline int
osmosis_dma_from_host(uint64_t host_addr, uint32_t nic_addr,
		      uint32_t length, bool generate_event,
		      osmosis_cmd_t *xfer)
{
    return spin_dma_from_host(host_addr, nic_addr, length, generate_event, xfer);
}
