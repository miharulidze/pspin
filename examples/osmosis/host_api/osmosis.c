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

#include "pspinsim.h"
#include "spin.h"
#include "../osmosis_args.h"
#include "osmosis.h"
#include "packets.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#define SLM_FILES "build/slm_files/"

#define MAGIC_PATH "NULL"

#define EC_MAX_NUM 2

#define NIC_L2_ADDR 0x1c300000
#define NIC_L2_SIZE (1024 * 1024)
#define NIC_L2_EC_CHUNK_SIZE (NIC_L2_SIZE / EC_MAX_NUM)

#define HOST_ADDR 0xdeadbeefdeadbeef
#define HOST_SIZE (1024 * 1024 * 1024)
#define HOST_EC_CHUNK_SIZE (HOST_SIZE / EC_MAX_NUM)

#define SCRATCHPAD_REL_ADDR 0
#define SCRATCHPAD_SIZE (800 * 1024)
#define SCRATCHPAD_EC_CHUNK_SIZE (SCRATCHPAD_SIZE / EC_MAX_NUM)

#define WAIT_CYCLES_PER_BYTE (0.02)

#define EC_MEM_BASE_ADDR(generic_ectx_id, base, chunk_size) \
    (base + (generic_ectx_id * chunk_size))

#define CHECK_ERR(S)                   \
    {                                  \
        int res;                       \
        if ((res = S) != SPIN_SUCCESS) \
            return res;                \
    }

typedef struct gdriver_ttrace
{
    char *trace_path;
    uint32_t packets_parsed;
    match_packet_fun_t matching_cb;
} gdriver_ttrace_t;

typedef struct gdriver_tgen
{
    uint32_t packet_size;
    uint32_t packet_delay;
    uint32_t message_delay;
    uint32_t packets_sent;
} gdriver_tgen_t;

typedef struct gdriver_ectx
{
    spin_ec_t ectx;
    gdriver_packet_filler_t pkt_filler;
    gdriver_ectx_conf_t user_conf;
    char matching_rule[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE];
} gdriver_ectx_t;

typedef struct gdriver_sim_descr
{
    int num_ectxs;
    gdriver_ectx_t ectxs[EC_MAX_NUM];

    int is_trace;
    gdriver_ttrace_t ttrace;
    gdriver_tgen_t tgen;

    uint32_t packets_processed;
} gdriver_sim_descr_t;

static gdriver_sim_descr_t sim_state;

static void gdriver_dump_ectx_info(const gdriver_ectx_t *gectx, uint32_t gectx_id)
{
    printf("[GDRIVER] gectx=%p id=%u info:\n", gectx, gectx_id);
    printf("[GDRIVER] hh_addr=%x; hh_size=%u;\n",
        gectx->ectx.hh_addr, gectx->ectx.hh_size);
    printf("[GDRIVER] ph_addr=%x; ph_size=%u;\n",
        gectx->ectx.ph_addr, gectx->ectx.ph_size);
    printf("[GDRIVER] th_addr=%x; th_size=%u;\n",
        gectx->ectx.th_addr, gectx->ectx.th_size);
    printf("[GDRIVER] handler_mem_addr=%x; handler_mem_size=%u\n",
        gectx->ectx.handler_mem_addr, gectx->ectx.handler_mem_size);
    printf("[GDRIVER] host_mem_addr=%lx; host_mem_size=%u\n",
        gectx->ectx.host_mem_addr, gectx->ectx.host_mem_size);
    printf("[GDRIVER] slo.compute=%u; slo.io=%u\n",
        gectx->user_conf.slo.compute_prio, gectx->user_conf.slo.io_prio);
    printf("[GDRIVER] matching context=%s\n", gectx->matching_rule ? gectx->matching_rule : "NULL");
}

static void gdriver_fill_pkt(int ectx_id, uint8_t *pkt_buf, uint32_t pkt_size)
{
    pkt_hdr_t *hdr;

    // IP + UDP header
    hdr = (pkt_hdr_t*)pkt_buf;
    hdr->ip_hdr.ihl = 5;
    hdr->ip_hdr.length = pkt_size;
    // TODO: fill UDP fields

    /* fill workload specific payload */
    if (sim_state.ectxs[ectx_id].pkt_filler.cb != NULL) {
        assert(pkt_size - sizeof(pkt_hdr_t) >= sim_state.ectxs[ectx_id].pkt_filler.size);
        sim_state.ectxs[ectx_id].pkt_filler.cb(pkt_buf, pkt_size, sim_state.ectxs[ectx_id].pkt_filler.data);
    }
}

static void gdriver_parse_trace()
{
    uint32_t nsources, npackets, max_pkt_size;
    uint32_t msgid, pkt_size, ipg, wait_cycles, l1_pkt_size;
    char src_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE];
    char dst_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE];
    uint8_t *pkt_buf;
    int is_last, matched, ret, ectx_id;

    FILE *trace_file = fopen(sim_state.ttrace.trace_path, "r");
    assert(trace_file);
    printf("[GDRIVER]: Packet trace path: %s\n", sim_state.ttrace.trace_path);

    /* Parse trace header */
    ret = fscanf(trace_file, "%u %u %u\n", &nsources, &npackets, &max_pkt_size);
     //assert(nsources > 0 && nsources == sim_state.num_ectxs);
    assert(npackets > 0);

    pkt_buf = (uint8_t *)malloc(sizeof(uint8_t) * max_pkt_size);
    assert(pkt_buf != NULL);

    sim_state.ttrace.packets_parsed = 0;

    while (!feof(trace_file)) {
        ret = fscanf(trace_file, "%s %s %u %u\n", src_addr, dst_addr, &pkt_size, &is_last);
        assert(ret == 4);

        matched = 0;
        for (ectx_id = 0; ectx_id < sim_state.num_ectxs; ectx_id++) {
            if (sim_state.ttrace.matching_cb(src_addr, dst_addr, sim_state.ectxs[ectx_id].matching_rule)) {
                matched = 1;
                break;
            }
        }

        assert(matched);

    printf("[GDRIVER]: pkt src=%s dst=%s pkt_size=%u is_last=%u matched_ectx=%u\n", src_addr, dst_addr, pkt_size, is_last, ectx_id);

        gdriver_fill_pkt(ectx_id, pkt_buf, pkt_size);
        wait_cycles = (int)(pkt_size * WAIT_CYCLES_PER_BYTE); /* + number of cycles determined by load */

        /* 1:1 mapping between FMQs and ECTXs */
        pspinsim_packet_add(&(sim_state.ectxs[ectx_id].ectx), ectx_id,
                            pkt_buf, pkt_size, pkt_size, is_last, wait_cycles, 0);

        sim_state.ttrace.packets_parsed++;
    }

    assert(sim_state.ttrace.packets_parsed == npackets);

    fclose(trace_file);
    free(pkt_buf);

    pspinsim_packet_eos();
}

static void gdriver_pcie_mst_write_complete(void *user_ptr)
{
    printf("Write to NIC memory completed (user_ptr: %p)\n", user_ptr);
}

static void gdriver_feedback(uint64_t user_ptr, uint64_t nic_arrival_time,
                 uint64_t pspin_arrival_time, uint64_t feedback_time)
{
    sim_state.packets_processed++;
}

static int gdriver_init_ectx(gdriver_ectx_t *gectx, uint32_t gectx_id,
                 const gdriver_ectx_conf_t *ectx_conf)
{
    spin_nic_addr_t hh_addr = 0, ph_addr = 0, th_addr = 0;
    size_t hh_size = 0, ph_size = 0, th_size = 0;

    if ((ectx_conf->handler.hh_name == NULL) &&
    (ectx_conf->handler.ph_name == NULL) &&
    (ectx_conf->handler.th_name == NULL))
        return GDRIVER_ERR;

    if (ectx_conf->handler.hh_name != NULL)
        CHECK_ERR(spin_find_handler_by_name(ectx_conf->handler.file_name,
                        ectx_conf->handler.hh_name,
                        &hh_addr, &hh_size));

    if (ectx_conf->handler.ph_name != NULL)
        CHECK_ERR(spin_find_handler_by_name(ectx_conf->handler.file_name,
                        ectx_conf->handler.ph_name,
                        &ph_addr, &ph_size));

    if (ectx_conf->handler.th_name != NULL)
        CHECK_ERR(spin_find_handler_by_name(ectx_conf->handler.file_name,
                        ectx_conf->handler.th_name,
                        &th_addr, &th_size));

    gectx->ectx.hh_addr = hh_addr;
    gectx->ectx.ph_addr = ph_addr;
    gectx->ectx.th_addr = th_addr;
    gectx->ectx.hh_size = hh_size;
    gectx->ectx.ph_size = ph_size;
    gectx->ectx.th_size = th_size;

    /*
     * For now assume that each execution context had its own region of host/L1/L2
     * memories.
     *
     * See EC_MEM_BASE_ADDR macro definition for clarity.
     */

    gectx->ectx.handler_mem_addr = EC_MEM_BASE_ADDR(gectx_id, NIC_L2_ADDR, NIC_L2_EC_CHUNK_SIZE);
    gectx->ectx.handler_mem_size = NIC_L2_EC_CHUNK_SIZE;
    assert(gectx->ectx.handler_mem_addr < (NIC_L2_ADDR + NIC_L2_SIZE));

    gectx->ectx.host_mem_addr = EC_MEM_BASE_ADDR(gectx_id, HOST_ADDR, HOST_EC_CHUNK_SIZE);
    gectx->ectx.host_mem_size = HOST_EC_CHUNK_SIZE;
    assert(gectx->ectx.host_mem_addr < (HOST_ADDR + HOST_SIZE));

    for (int i = 0; i < NUM_CLUSTERS; i++) {
        gectx->ectx.scratchpad_addr[i] = EC_MEM_BASE_ADDR(gectx_id, SCRATCHPAD_REL_ADDR, SCRATCHPAD_EC_CHUNK_SIZE);
        gectx->ectx.scratchpad_size[i] = SCRATCHPAD_EC_CHUNK_SIZE;
        assert(gectx->ectx.scratchpad_addr[i] < (SCRATCHPAD_REL_ADDR + SCRATCHPAD_SIZE));
    }

    if (ectx_conf->handler_l2_img.ptr) {
        assert(ectx_conf->handler_l2_img.size <= gectx->ectx.handler_mem_size);
        spin_nicmem_write(gectx->ectx.handler_mem_addr,
              ectx_conf->handler_l2_img.ptr,
              ectx_conf->handler_l2_img.size,
              (void *)0);
    }

    memset(&gectx->pkt_filler, 0, sizeof(gectx->pkt_filler));

    if (ectx_conf->fmq_matching_rule.ptr)
        strcpy(gectx->matching_rule, ectx_conf->fmq_matching_rule.ptr);

    pspinsim_set_fmq_prio(gectx_id, ectx_conf->slo.compute_prio);

    memcpy(&gectx->user_conf, ectx_conf, sizeof(gectx->user_conf));
    gdriver_dump_ectx_info(gectx, gectx_id);

    return GDRIVER_OK;
}

int gdriver_add_ectx(const gdriver_ectx_conf_t *ectx_conf)
{
    int ret;

    if (sim_state.num_ectxs == EC_MAX_NUM)
        return -1;

    if (ectx_conf->fmq_matching_rule.size > GDRIVER_FMQ_MATCHING_RULE_MAXSIZE)
        return -1;

    if (gdriver_init_ectx(&(sim_state.ectxs[sim_state.num_ectxs]), sim_state.num_ectxs, ectx_conf))
        return -1;

    return sim_state.num_ectxs++;
}

int gdriver_add_pkt_filler(int ectx_id, const gdriver_packet_filler_t *filler)
{
    if (!filler)
        return -1;

    memcpy(&sim_state.ectxs[ectx_id].pkt_filler, filler, sizeof(gdriver_packet_filler_t));
    return GDRIVER_OK;
}

int gdriver_run()
{
    gdriver_parse_trace();

    if (pspinsim_run() == SPIN_SUCCESS) {
        return GDRIVER_OK;
    } else {
        return GDRIVER_ERR;
    }
}

int gdriver_fini()
{
    if (pspinsim_fini() != SPIN_SUCCESS)
        return GDRIVER_ERR;

    if (sim_state.is_trace && sim_state.ttrace.packets_parsed != sim_state.packets_processed)
        return GDRIVER_ERR;

    else if (sim_state.tgen.packets_sent != sim_state.packets_processed)
        return GDRIVER_ERR;

    return GDRIVER_OK;
}

int gdriver_init(int argc, char **argv, match_packet_fun_t matching_cb, int *ectx_num)
{
    struct gengetopt_args_info ai;
    pspin_conf_t conf;

    *ectx_num = -1;

    if (cmdline_parser(argc, argv, &ai) != 0)
        return GDRIVER_ERR;

    pspinsim_default_conf(&conf);
    conf.slm_files_path = SLM_FILES;

    pspinsim_init(argc, argv, &conf);

    pspinsim_cb_set_pcie_mst_write_completion(gdriver_pcie_mst_write_complete);
    pspinsim_cb_set_pkt_feedback(gdriver_feedback);

    memset(&sim_state, 0, sizeof(sim_state));

    if (strcmp(ai.trace_file_arg, MAGIC_PATH)) {
        sim_state.is_trace = 1;
        sim_state.ttrace.trace_path = ai.trace_file_arg;
        sim_state.ttrace.matching_cb = matching_cb;
    } else {
        sim_state.tgen.packet_size = ai.packet_size_arg;
        sim_state.tgen.packet_delay = ai.packet_delay_arg;
        sim_state.tgen.message_delay = ai.message_delay_arg;
    }

    *ectx_num = EC_MAX_NUM;

    return GDRIVER_OK;
}
