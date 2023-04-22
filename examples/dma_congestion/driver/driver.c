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
#include "../handlers/dma.h"

int match_ectx_cb(void *src, void *dst, void *ectx_addr)
{
    if (!strcmp((char *)dst, (char*)ectx_addr)) {
        printf("DEBUG: src=%s matching_ctx=%s\n", (char *)dst, (char *)ectx_addr);
        return 1;
    }
    return 0;
}

int fill_benchmark_params(void *pkt, void *params) {
    benchmark_params_t *dst = (benchmark_params_t *)((char *)pkt + sizeof(pkt_hdr_t));
    benchmark_params_t *src = (benchmark_params_t *)params;
    memcpy((void *)dst, params, sizeof(benchmark_params_t));
}

int main(int argc, char **argv)
{
    int ectx_num, victim_ectx_id, attacker_ectx_id, ret;

    const char *handlers_file = "build/dma";
    const char *hh = NULL;
    const char *ph = "dma_ph";
    const char *th = NULL;

    if (gdriver_init(argc, argv, match_ectx_cb, &ectx_num) != GDRIVER_OK)
        return EXIT_FAILURE;

    /* victim --> (ectx #0) --> 192.168.0.0 */
    char victim_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.0";
    benchmark_params_t victim_params;
    victim_params.dma_count = 1;
    victim_params.dma_read_size = 64; // victim generates 1 64B read per each packet

    gdriver_ectx_conf_t victim_conf;
    memset((void *)&victim_conf, 0, sizeof(gdriver_ectx_conf_t));
    victim_conf.handler.file_name = handlers_file;
    victim_conf.handler.hh_name = hh;
    victim_conf.handler.ph_name = ph;
    victim_conf.handler.th_name = th;
    victim_conf.fmq_matching_rule.ptr = victim_addr;
    victim_conf.fmq_matching_rule.size = strlen(victim_addr) + 1;
    victim_conf.slo.compute_prio = 1;

    victim_ectx_id = gdriver_add_ectx(&victim_conf);
    if (victim_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    gdriver_packet_filler_t victim_pkt_filler;
    memset((void *)&victim_pkt_filler, 0, sizeof(gdriver_packet_filler_t));
    victim_pkt_filler.cb = fill_benchmark_params;
    victim_pkt_filler.data = &victim_params;
    victim_pkt_filler.size = sizeof(benchmark_params_t);
    gdriver_add_pkt_filler(victim_ectx_id, &victim_pkt_filler);

    /* attacker --> (ectx #2) --> 192.168.0.1 */
    char attacker_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.1";
    benchmark_params_t attacker_params;
    attacker_params.dma_count = 8;
    attacker_params.dma_read_size = 1024; // attacker generates 1 1024B read per each packet

    gdriver_ectx_conf_t attacker_conf;
    memset((void *)&attacker_conf, 0, sizeof(gdriver_ectx_conf_t));
    attacker_conf.handler.file_name = handlers_file;
    attacker_conf.handler.hh_name = hh;
    attacker_conf.handler.ph_name = ph;
    attacker_conf.handler.th_name = th;
    attacker_conf.fmq_matching_rule.ptr = attacker_addr;
    attacker_conf.fmq_matching_rule.size = strlen(attacker_addr) + 1;
    attacker_conf.slo.compute_prio = 1;

    attacker_ectx_id = gdriver_add_ectx(&attacker_conf);
    if (attacker_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    gdriver_packet_filler_t attacker_pkt_filler;
    memset((void *)&attacker_pkt_filler, 0, sizeof(gdriver_packet_filler_t));
    attacker_pkt_filler.cb = fill_benchmark_params;
    attacker_pkt_filler.data = &attacker_params;
    attacker_pkt_filler.size = sizeof(benchmark_params_t);
    gdriver_add_pkt_filler(attacker_ectx_id, &attacker_pkt_filler);

    if (gdriver_run() != GDRIVER_OK)
        return EXIT_FAILURE;

    return (gdriver_fini() == GDRIVER_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
