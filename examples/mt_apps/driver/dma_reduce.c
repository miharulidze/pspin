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

#include <string.h>
#include <stdio.h>

#include <packets.h>
#include "osmosis.h"

#include "../handlers/kernels.h"

int match_ectx_cb(void *src, void *dst, void *ectx_addr)
{
    if (!strcmp((char *)dst, (char*)ectx_addr))
        return 1;
    return 0;
}

int fill_benchmark_params(void *pkt, void *params) {
    benchmark_params_t *dst = (benchmark_params_t *)((char *)pkt + sizeof(pkt_hdr_t));
    benchmark_params_t *src = (benchmark_params_t *)params;
    memcpy((void *)dst, params, sizeof(benchmark_params_t));
}

int main(int argc, char **argv)
{
    int ectx_num, dma_ectx_id, reduce_ectx_id, ret;

    const char *dma_handlers_file = "build/kernels";
    const char *dma_hh = NULL;
    const char *dma_ph = "dma_l2_read_ph";
    const char *dma_th = NULL;

    if (gdriver_init(argc, argv, match_ectx_cb, &ectx_num) != GDRIVER_OK)
        return EXIT_FAILURE;

    /* dma --> (ectx #0) --> 192.168.0.0 */
    char dma_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.0";
    benchmark_params_t dma_params;
    dma_params.dma_count = 1;
    dma_params.dma_read_size = 64; // dma generates 1 64B read per each packet

    gdriver_ectx_conf_t dma_conf;
    memset((void *)&dma_conf, 0, sizeof(gdriver_ectx_conf_t));
    dma_conf.handler.file_name = dma_handlers_file;
    dma_conf.handler.hh_name = dma_hh;
    dma_conf.handler.ph_name = dma_ph;
    dma_conf.handler.th_name = dma_th;
    dma_conf.fmq_matching_rule.ptr = dma_addr;
    dma_conf.fmq_matching_rule.size = strlen(dma_addr) + 1;
    dma_conf.slo.compute_prio = 1;

    dma_ectx_id = gdriver_add_ectx(&dma_conf);
    if (dma_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    gdriver_packet_filler_t dma_pkt_filler;
    memset((void *)&dma_pkt_filler, 0, sizeof(gdriver_packet_filler_t));
    dma_pkt_filler.cb = fill_benchmark_params;
    dma_pkt_filler.data = &dma_params;
    dma_pkt_filler.size = sizeof(benchmark_params_t);
    gdriver_add_pkt_filler(dma_ectx_id, &dma_pkt_filler);

    /* reduce --> (ectx #2) --> 192.168.0.1 */
    const char *reduce_handlers_file = "build/kernels";
    const char *reduce_hh = NULL;
    const char *reduce_ph = "reduce_l1_ph";
    const char *reduce_th = NULL;
    char reduce_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.1";
    //benchmark_params_t reduce_params;
    //reduce_params.dma_count = 8;
    //reduce_params.dma_read_size = 1024; // reduce generates 1 1024B read per each packet

    gdriver_ectx_conf_t reduce_conf;
    memset((void *)&reduce_conf, 0, sizeof(gdriver_ectx_conf_t));
    reduce_conf.handler.file_name = reduce_handlers_file;
    reduce_conf.handler.hh_name = reduce_hh;
    reduce_conf.handler.ph_name = reduce_ph;
    reduce_conf.handler.th_name = reduce_th;
    reduce_conf.fmq_matching_rule.ptr = reduce_addr;
    reduce_conf.fmq_matching_rule.size = strlen(reduce_addr) + 1;
    reduce_conf.slo.compute_prio = 2;

    reduce_ectx_id = gdriver_add_ectx(&reduce_conf);
    if (reduce_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    //gdriver_packet_filler_t reduce_pkt_filler;
    //memset((void *)&reduce_pkt_filler, 0, sizeof(gdriver_packet_filler_t));
    //reduce_pkt_filler.cb = fill_benchmark_params;
    //reduce_pkt_filler.data = &reduce_params;
    //reduce_pkt_filler.size = sizeof(benchmark_params_t);
    //gdriver_add_pkt_filler(reduce_ectx_id, &reduce_pkt_filler);

    if (gdriver_run() != GDRIVER_OK)
        return EXIT_FAILURE;

    return (gdriver_fini() == GDRIVER_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
