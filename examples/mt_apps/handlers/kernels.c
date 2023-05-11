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

#define DEFAULT_CHUNK_SIZE 512
#define DIVISOR 512

#include "../../osmosis/handler_api/osmosis_io.h"
#include "kernels.h"

/*
 * Synthetic kernels
 */

__handler__ void spinner_ph(handler_args_t *args)
{
    task_t *task = args->task;
    char *payload_ptr = (char *)task->pkt_mem + sizeof(pkt_hdr_t);
    io_params_t params = *(io_params_t *)payload_ptr;
    volatile int xx = 0;
    int x = xx;

    //printf("spinner ph %d\n", params.io_reqs_count);

    for (int i = 0; i < params.io_reqs_count; i++) {
        x = x * i;
    }

    xx = x;
}

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

/* Real apps: IO-bound */

__handler__ void io_write_ph(handler_args_t *args)
{
    task_t* task = args->task;
    ip_hdr_t *ip_hdr = (ip_hdr_t*) (task->pkt_mem);
    uint8_t *nic_pld_addr = ((uint8_t*) (task->pkt_mem));
    uint16_t pkt_pld_len = ip_hdr->length;
    spin_cmd_t dma;

    uint64_t host_address = task->host_mem_high;
    host_address = (host_address << 32) | (task->host_mem_low);
    spin_dma_to_host(host_address, (uint32_t) nic_pld_addr, pkt_pld_len, 1, &dma);
    spin_cmd_wait(dma);
}

__handler__ void osmosis_io_write_ph(handler_args_t *args)
{
    task_t* task = args->task;
    ip_hdr_t *ip_hdr = (ip_hdr_t*) (task->pkt_mem);
    uint8_t *nic_pld_addr = ((uint8_t*) (task->pkt_mem));
    uint16_t pkt_pld_len = ip_hdr->length;
    osmosis_cmd_t dma;

    uint64_t host_address = task->host_mem_high;
    host_address = (host_address << 32) | (task->host_mem_low);
    osmosis_dma_to_host(host_address, (uint32_t) nic_pld_addr, pkt_pld_len, 1, &dma, DEFAULT_CHUNK_SIZE);
    osmosis_cmd_wait(dma);
}

__handler__ void io_read_ph(handler_args_t *args)
{
    task_t* task = args->task;
    ip_hdr_t *ip_hdr = (ip_hdr_t*) (task->pkt_mem);
    uint8_t *nic_pld_addr = ((uint8_t*) (task->pkt_mem));
    uint16_t pkt_pld_len = ip_hdr->length;
    udp_hdr_t *udp_hdr = (udp_hdr_t*) (((uint8_t*) (task->pkt_mem)) + ip_hdr->ihl * 4);

    uint32_t src_id = ip_hdr->source_id;
    ip_hdr->source_id = ip_hdr->dest_id;
    ip_hdr->dest_id = src_id;

    uint16_t src_port = udp_hdr->src_port;
    udp_hdr->src_port = udp_hdr->dst_port;
    udp_hdr->dst_port = src_port;

    spin_cmd_t send;
    spin_cmd_t dma;
    uint64_t host_address = task->host_mem_high;
    host_address = (host_address << 32) | (task->host_mem_low);

    uint32_t n_reqs = pkt_pld_len >> 10; // pkt_pld_len/1024
    n_reqs += 1;

    for (; n_reqs > 0; n_reqs--) {
        spin_dma_from_host(host_address, (uint32_t) nic_pld_addr, pkt_pld_len, 1, &dma);
        spin_cmd_wait(dma);
        spin_send_packet(nic_pld_addr, pkt_pld_len, &send);
        spin_cmd_wait(send);
    }
}

__handler__ void osmosis_io_read_ph(handler_args_t *args)
{
    task_t* task = args->task;
    ip_hdr_t *ip_hdr = (ip_hdr_t*) (task->pkt_mem);
    uint8_t *nic_pld_addr = ((uint8_t*) (task->pkt_mem));
    uint32_t pkt_pld_len = ip_hdr->length;
    udp_hdr_t *udp_hdr = (udp_hdr_t*) (((uint8_t*) (task->pkt_mem)) + ip_hdr->ihl * 4);

    uint32_t src_id = ip_hdr->source_id;
    ip_hdr->source_id = ip_hdr->dest_id;
    ip_hdr->dest_id = src_id;

    uint16_t src_port = udp_hdr->src_port;
    udp_hdr->src_port = udp_hdr->dst_port;
    udp_hdr->dst_port = src_port;

    spin_cmd_t send;
    osmosis_cmd_t dma;
    uint64_t host_address = task->host_mem_high;
    host_address = (host_address << 32) | (task->host_mem_low);

    uint32_t n_reqs = pkt_pld_len >> 10;
    n_reqs += 1;

    for (; n_reqs > 0; n_reqs--) {
        osmosis_dma_from_host(host_address, (uint32_t) nic_pld_addr, pkt_pld_len, 1, &dma, DEFAULT_CHUNK_SIZE);
        osmosis_cmd_wait(dma);
        spin_send_packet(nic_pld_addr, pkt_pld_len, &send);
        spin_cmd_wait(send);
    }
}

__handler__ void filtering_ph(handler_args_t *args)
{
    task_t* task = args->task;

    uint32_t *mem = (uint32_t *) task->handler_mem;
    uint8_t *key_byte = (uint8_t*) task->pkt_mem;

    uint32_t hash;
    HASH_JEN(key_byte, KEY_SIZE, hash);
    hash = FAST_MOD(hash, TOT_WORDS);

    *((uint32_t *) task->l2_pkt_mem) = mem[hash];

    spin_cmd_t cpy;
    uint64_t host_address = task->host_mem_high;
    host_address = (host_address << 32) | (task->host_mem_low);
    spin_dma_to_host(host_address, (uint32_t)task->l2_pkt_mem, task->pkt_mem_size, 0, &cpy);
    spin_cmd_wait(cpy);
}

__handler__ void osmosis_filtering_ph(handler_args_t *args)
{
    task_t* task = args->task;

    uint32_t *mem = (uint32_t *) task->handler_mem;
    uint8_t *key_byte = (uint8_t*) task->pkt_mem;

    uint32_t hash;
    HASH_JEN(key_byte, KEY_SIZE, hash);
    hash = FAST_MOD(hash, TOT_WORDS);

    *((uint32_t *) task->l2_pkt_mem) = mem[hash];

    osmosis_cmd_t cpy;
    uint64_t host_address = task->host_mem_high;
    host_address = (host_address << 32) | (task->host_mem_low);
    osmosis_dma_to_host(host_address, (uint32_t)task->l2_pkt_mem, task->pkt_mem_size, 0, &cpy, DEFAULT_CHUNK_SIZE);
    osmosis_cmd_wait(cpy);
}

/* Real-apps: Compute-bound */

__handler__ void aggregate_global_ph(handler_args_t *args)
{
    task_t* task = args->task;
    uint32_t *scratchpad = (uint32_t *)task->scratchpad;

    uint8_t *pkt_pld_ptr;
    uint32_t pkt_pld_len;
    GET_IP_UDP_PLD(task->pkt_mem, pkt_pld_ptr, pkt_pld_len);

    uint32_t *nic_pld_addr = (uint32_t*) pkt_pld_ptr;

    uint32_t aggregator=0;
    for (uint32_t i=0; i < pkt_pld_len / 4; i++)
        aggregator+=nic_pld_addr[i*STRIDE+OFFSET];

    uint32_t my_cluster_id = args->cluster_id;
    amo_add(&(scratchpad[my_cluster_id]), aggregator);
}

__handler__ void osmosis_aggregate_global_ph(handler_args_t *args)
{
    task_t* task = args->task;
    uint32_t *scratchpad = (uint32_t *)task->scratchpad;

    uint8_t *pkt_pld_ptr;
    uint32_t pkt_pld_len;
    GET_IP_UDP_PLD(task->pkt_mem, pkt_pld_ptr, pkt_pld_len);

    uint32_t *nic_pld_addr = (uint32_t*) pkt_pld_ptr;

    uint32_t aggregator=0;
    for (uint32_t i=0; i < pkt_pld_len / 4; i++)
        aggregator+=nic_pld_addr[i*STRIDE+OFFSET];

    uint32_t my_cluster_id = args->cluster_id;
    amo_add(&(scratchpad[my_cluster_id]), aggregator);
}

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

__handler__ void osmosis_reduce_l1_ph(handler_args_t *args)
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

__handler__ void histogram_l1_ph(handler_args_t *args)
{
    task_t* task = args->task;

    uint8_t *pkt_pld_ptr;
    uint32_t pkt_pld_len;
    GET_IP_UDP_PLD(task->pkt_mem, pkt_pld_ptr, pkt_pld_len);

    int32_t *nic_pld_addr = (int32_t*) pkt_pld_ptr;
    int32_t *local_mem = (int32_t*) (task->scratchpad[args->cluster_id]);
    volatile int32_t* word_ptr = &(local_mem[nic_pld_addr[0]]);

    for (uint32_t i = 1; i < pkt_pld_len / 4; i++)
    {
        amo_add(word_ptr, 1);
        word_ptr = &(local_mem[nic_pld_addr[i]]);
    }
}

__handler__ void osmosis_histogram_l1_ph(handler_args_t *args)
{
    task_t* task = args->task;

    uint8_t *pkt_pld_ptr;
    uint32_t pkt_pld_len;
    GET_IP_UDP_PLD(task->pkt_mem, pkt_pld_ptr, pkt_pld_len);

    int32_t *nic_pld_addr = (int32_t*) pkt_pld_ptr;

    int32_t *local_mem = (int32_t*) (task->scratchpad[args->cluster_id]);

    volatile int32_t* word_ptr = &(local_mem[nic_pld_addr[0]]);

    //we assume the number of msg size divides the pkt payload size
    for (uint32_t i = 1; i < pkt_pld_len / 4; i++)
    {
        amo_add(word_ptr, 1);
        word_ptr = &(local_mem[nic_pld_addr[i]]);
    }
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
    volatile handler_fn handlers[] = {NULL, reduce_l1_ph, NULL};
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
    volatile handler_fn handlers[] = {NULL, osmosis_send_packet_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers10(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, spinner_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers11(handler_fn * hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, io_read_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers12(handler_fn * hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, io_write_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers13(handler_fn * hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, aggregate_global_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers14(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, filtering_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers15(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, histogram_l1_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers16(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, osmosis_filtering_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers17(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, osmosis_io_write_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers18(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, osmosis_io_read_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers19(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, osmosis_aggregate_global_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers20(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, osmosis_reduce_l1_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}

void init_handlers21(handler_fn *hh, handler_fn *ph, handler_fn *th, void **handler_mem_ptr)
{
    volatile handler_fn handlers[] = {NULL, osmosis_histogram_l1_ph, NULL};
    *hh = handlers[0];
    *ph = handlers[1];
    *th = handlers[2];
}
