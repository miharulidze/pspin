// Copyright 2022 ETH Zurich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef HOST
#include <handler.h>
#include <packets.h>
#else
#include <handler_profiler.h>
#endif

#include "../../osmosis/handler_api/osmosis_io.h"
#include "kernels.h"

__handler__ void dma_l2_read_ph(handler_args_t *args)
{
    task_t *task = args->task;
    char *payload_ptr = (char *)task->pkt_mem + sizeof(pkt_hdr_t);
    io_params_t params = *(io_params_t *)payload_ptr;
    char *src_addr = (char *)task->handler_mem;
    spin_dma_t dma;

    //printf("dma_l2_read ph io_req_len=%u io_reqs_count=%d\n",
    //       params.io_req_len, params.io_reqs_count);

    for (int i = 0; i < params.io_reqs_count; i++) {
        spin_dma((void *)src_addr, (void *)payload_ptr,
                 params.io_req_len, 0, 0, &dma);
        spin_dma_wait(dma);
    }
}

__handler__ void osmosis_dma_l2_read_ph(handler_args_t *args)
{
    task_t *task = args->task;
    char *payload_ptr = (char *)task->pkt_mem + sizeof(pkt_hdr_t);
    io_params_t params = *(io_params_t *)payload_ptr;
    char *src_addr = (char *)task->handler_mem;
    osmosis_dma_t dma;

    //printf("osmosis_dma_l2_read ph io_req_len=%u io_reqs_count=%d\n",
    //       params.io_req_len, params.io_reqs_count);

    for (int i = 0; i < params.io_reqs_count; i++) {
        osmosis_dma((void *)src_addr, (void *)payload_ptr,
                    params.io_req_len, 0, 0, &dma,
                    params.io_chunk_size);
        osmosis_dma_wait(dma);
    }
}

__handler__ void dma_to_host_ph(handler_args_t *args)
{
    task_t *task = args->task;
    char *payload_ptr = (char *)task->pkt_mem + sizeof(pkt_hdr_t);
    io_params_t params = *(io_params_t *)payload_ptr;
    spin_cmd_t cmd;

    //printf("dma_to_host ph io_req_len=%u io_reqs_count=%d\n",
    //       params.io_req_len, params.io_reqs_count);

    uint64_t host_address = task->host_mem_high;
    host_address = (host_address << 32) | (task->host_mem_low);

    for (int i = 0; i < params.io_reqs_count; i++) {
        spin_dma_to_host(host_address, (uint32_t) payload_ptr,
                         params.io_req_len, 1, &cmd);
        spin_cmd_wait(cmd);
    }
}

__handler__ void osmosis_dma_to_host_ph(handler_args_t *args)
{
    task_t *task = args->task;
    char *payload_ptr = (char *)task->pkt_mem + sizeof(pkt_hdr_t);
    io_params_t params = *(io_params_t *)payload_ptr;
    osmosis_cmd_t cmd;

    //printf("osmosis_dma_to_host ph io_req_len=%u io_reqs_count=%d\n",
    //       params.io_req_len, params.io_reqs_count);

    uint64_t host_address = task->host_mem_high;
    host_address = (host_address << 32) | (task->host_mem_low);

    for (int i = 0; i < params.io_reqs_count; i++) {
        osmosis_dma_to_host(host_address, (uint32_t) payload_ptr,
                            params.io_req_len, 1, &cmd,
                            params.io_chunk_size);
        osmosis_cmd_wait(cmd);
    }
}

__handler__ void dma_from_host_ph(handler_args_t *args)
{
    task_t *task = args->task;
    char *payload_ptr = (char *)task->pkt_mem + sizeof(pkt_hdr_t);
    io_params_t params = *(io_params_t *)payload_ptr;
    spin_cmd_t cmd;

    //printf("dma_from_host ph io_req_len=%u io_reqs_count=%d\n",
    //       params.io_req_len, params.io_reqs_count);

    uint64_t host_address = task->host_mem_high;
    host_address = (host_address << 32) | (task->host_mem_low);

    for (int i = 0; i < params.io_reqs_count; i++) {
        spin_dma_from_host(host_address, (uint32_t) payload_ptr,
                           params.io_req_len, 1, &cmd);
        spin_cmd_wait(cmd);
    }
}

__handler__ void osmosis_dma_from_host_ph(handler_args_t *args)
{
    task_t *task = args->task;
    char *payload_ptr = (char *)task->pkt_mem + sizeof(pkt_hdr_t);
    io_params_t params = *(io_params_t *)payload_ptr;
    osmosis_cmd_t cmd;

    //printf("osmosis_dma_from_host ph io_req_len=%u io_reqs_count=%d\n",
    //       params.io_req_len, params.io_reqs_count);

    uint64_t host_address = task->host_mem_high;
    host_address = (host_address << 32) | (task->host_mem_low);

    for (int i = 0; i < params.io_reqs_count; i++) {
        osmosis_dma_from_host(host_address, (uint32_t) payload_ptr,
                              params.io_req_len, 1, &cmd,
                              params.io_chunk_size);
        osmosis_cmd_wait(cmd);
    }
}

__handler__ void send_packet_ph(handler_args_t *args)
{
    task_t *task = args->task;
    char *payload_ptr = (char *)task->pkt_mem + sizeof(pkt_hdr_t);
    io_params_t params = *(io_params_t *)payload_ptr;
    ip_hdr_t *ip_hdr_ptr = (ip_hdr_t *)task->pkt_mem;
    udp_hdr_t *udp_hdr_ptr = (udp_hdr_t *)((char *)task->pkt_mem + sizeof(ip_hdr_t));
    spin_cmd_t cmd;

    //printf("send_packet ph io_req_len=%u io_reqs_count=%d\n",
    //       params.io_req_len, params.io_reqs_count);

    uint32_t src_id = ip_hdr_ptr->source_id;
    ip_hdr_ptr->source_id = ip_hdr_ptr->dest_id;
    ip_hdr_ptr->dest_id = src_id;

    uint16_t src_port = udp_hdr_ptr->src_port;
    udp_hdr_ptr->src_port = udp_hdr_ptr->dst_port;
    udp_hdr_ptr->dst_port = src_port;

    for (int i = 0; i < params.io_reqs_count; i++) {
        spin_send_packet(payload_ptr, params.io_req_len, &cmd);
        spin_cmd_wait(cmd);
    }
}

__handler__ void osmosis_send_packet_ph(handler_args_t *args)
{
    task_t *task = args->task;
    char *payload_ptr = (char *)task->pkt_mem + sizeof(pkt_hdr_t);
    io_params_t params = *(io_params_t *)payload_ptr;
    ip_hdr_t *ip_hdr_ptr = (ip_hdr_t *)task->pkt_mem;
    udp_hdr_t *udp_hdr_ptr = (udp_hdr_t *)((char *)task->pkt_mem + sizeof(ip_hdr_t));
    spin_cmd_t cmd;

    //printf("osmosis_send_packet ph io_req_len=%u io_reqs_count=%d\n",
    //       params.io_req_len, params.io_reqs_count);

    uint32_t src_id = ip_hdr_ptr->source_id;
    ip_hdr_ptr->source_id = ip_hdr_ptr->dest_id;
    ip_hdr_ptr->dest_id = src_id;

    uint16_t src_port = udp_hdr_ptr->src_port;
    udp_hdr_ptr->src_port = udp_hdr_ptr->dst_port;
    udp_hdr_ptr->dst_port = src_port;

    for (int i = 0; i < params.io_reqs_count; i++) {
        osmosis_send_packet(payload_ptr, params.io_req_len, &cmd,
                            params.io_chunk_size);
        osmosis_cmd_wait(cmd);
    }
}

/* reduce_l1 */

#define NUM_CLUSTERS 4
#define STRIDE 1
#define OFFSET 0
#define NUM_INT_OP 0

#define ZEROS 2048

// Handler that implements reduce in scratchpad for int32
// 4 uint32_t -> locks
// 4 uint32_t -> L1 addresses
// 1 uint32_t -> msg_count (now 0x0200)

__handler__ void reduce_l1_ph(handler_args_t *args)
{

    task_t* task = args->task;

    uint8_t *pkt_pld_ptr;
    uint32_t pkt_pld_len;
    GET_IP_UDP_PLD(task->pkt_mem, pkt_pld_ptr, pkt_pld_len);

    uint32_t *nic_pld_addr = (uint32_t*) pkt_pld_ptr;

    //reduce_mem_t *mem = (reduce_mem_t *)args->her->match_info.handler_mem;
    volatile int32_t *local_mem = (int32_t *)(task->scratchpad[args->cluster_id]);

    //printf("reduce_l1 ph\n");

    // we assume the number of msg size divides the pkt payload size
    for (uint32_t i = 0; i < pkt_pld_len / 4; i++)
    {
        amo_add(&(local_mem[i]), nic_pld_addr[i]);
        // We do need atomics here, as each handler writes to the
        // same adress as others in the same cluster.
    }
}

__handler__ void reduce_l1_th(handler_args_t *args)
{
    task_t* task = args->task;
    uint64_t host_address = task->host_mem_high;
    host_address = (host_address << 32) | (task->host_mem_low);

    //signal that we completed so to let the host read the result back
    spin_host_write(host_address, (uint64_t) 1, false);
}

__handler__ void spinner_ph(handler_args_t *args)
{
    task_t *task = args->task;
    char *payload_ptr = (char *)task->pkt_mem + sizeof(pkt_hdr_t);
    io_params_t params = *(io_params_t *)payload_ptr;
    volatile int xx = 0;
    int x = xx;

    //printf("spinner ph %d\n", params.io_reqs_count);

    for (int i = 0; i < params.io_reqs_count; i++) {
        x = x*i;
    }

    xx = x;
}

void init_handlers(handler_fn * hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, dma_l2_read_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers2(handler_fn * hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, osmosis_dma_l2_read_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers3(handler_fn * hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, dma_to_host_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers4(handler_fn * hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, osmosis_dma_to_host_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers5(handler_fn * hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, dma_from_host_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers6(handler_fn * hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, osmosis_dma_from_host_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers7(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, reduce_l1_ph, reduce_l1_th};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers8(handler_fn * hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, send_packet_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers9(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, osmosis_send_packet_ph, reduce_l1_th};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers10(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, spinner_ph, reduce_l1_th};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}
