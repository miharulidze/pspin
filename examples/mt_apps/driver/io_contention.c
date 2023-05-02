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
#include <assert.h>

#include <packets.h>
#include "osmosis.h"
#include "../handlers/kernels.h"
#include "../utils/utils.h"

typedef struct bmark_params {
    const char *handler_file;
    char *hh;
    char *ph;
    char *th;
    uint32_t victim_req_len;
    int victim_reqs_count;
    uint32_t attacker_req_len;
    int attacker_reqs_count;
    uint32_t io_chunk_size;
} bmark_params_t;

#define MAX_STRING_LEN 1024
const char *handler_file = "build/kernels";
char hh[MAX_STRING_LEN];
char ph[MAX_STRING_LEN];
char th[MAX_STRING_LEN];

// ./sim_io_congestion -t <trace_name> <test_type> <victim_io_size> <victim_io_n> <attacker_io_size> <attacker_io_n>
int bmark_parse_arguments(int argc, char **argv, bmark_params_t *bmark)
{
    if (argc != 9) {
        printf("bad arguments: "
               "./sim_io_congestion -t <trace_name> <test_type> <victim_io_size> <victim_io_n> <attacker_io_size> <attacker_io_n> <io_chunk_size>\n");
        assert(0);
    }

    bmark->handler_file = handler_file;
    bmark->hh = NULL;
    bmark->ph = argv[3];
    bmark->th = NULL;
    bmark->victim_req_len = atoi(argv[4]);
    bmark->victim_reqs_count = atoi(argv[5]);
    bmark->attacker_req_len = atoi(argv[6]);
    bmark->attacker_reqs_count = atoi(argv[7]);
    bmark->io_chunk_size = atoi(argv[8]);

    printf("Kernel=%s "
           "Victim req_len=%d req_count=%d "
           "Attacker req_len=%d req_count=%d io_chunk_size=%d\n",
           bmark->ph,
           bmark->victim_req_len, bmark->victim_reqs_count,
           bmark->attacker_req_len, bmark->attacker_reqs_count,
           bmark->io_chunk_size);

    return GDRIVER_OK;
}

int fill_benchmark_params_cb(void *pkt, size_t pkt_size, void *params) {
    io_params_t *dst = (io_params_t *)((char *)pkt + sizeof(pkt_hdr_t));
    io_params_t *src = (io_params_t *)params;
    memcpy((void *)dst, params, sizeof(io_params_t));
    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    int ectx_num;
    int victim_ectx_id, attacker_ectx_id;
    bmark_params_t bmark;

    memset(&bmark, 0, sizeof(bmark_params_t));
    assert(bmark_parse_arguments(argc, argv, &bmark) == GDRIVER_OK);
    assert(gdriver_init(argc, argv, match_ectx_cb, &ectx_num) == GDRIVER_OK);

    /* victim --> (ectx #0) --> 192.168.0.0 */

    io_params_t victim_params;
    victim_params.io_req_len = bmark.victim_req_len; // victim generates 1 64B read per each packet
    victim_params.io_reqs_count = bmark.victim_reqs_count;
    victim_params.io_chunk_size = bmark.io_chunk_size;

    gdriver_ectx_conf_t victim_conf;
    memset((void *)&victim_conf, 0, sizeof(gdriver_ectx_conf_t));
    victim_conf.handler.file_name = bmark.handler_file;
    victim_conf.handler.hh_name = bmark.hh;
    victim_conf.handler.ph_name = bmark.ph;
    victim_conf.handler.th_name = bmark.th;

    char victim_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.0";
    victim_conf.fmq_matching_rule.ptr = victim_addr;
    victim_conf.fmq_matching_rule.size = strlen(victim_addr) + 1;
    victim_conf.slo.compute_prio = 0;

    victim_ectx_id = gdriver_add_ectx(&victim_conf);
    if (victim_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    gdriver_packet_filler_t victim_pkt_filler;
    memset((void *)&victim_pkt_filler, 0, sizeof(gdriver_packet_filler_t));
    victim_pkt_filler.cb = fill_benchmark_params_cb;
    victim_pkt_filler.data = &victim_params;
    victim_pkt_filler.size = sizeof(io_params_t);
    gdriver_add_pkt_filler(victim_ectx_id, &victim_pkt_filler);

    /* attacker --> (ectx #1) --> 192.168.0.1 */

    io_params_t attacker_params;
    attacker_params.io_req_len = bmark.attacker_req_len; // attacker generates 1 1024B read per each packet
    attacker_params.io_reqs_count = bmark.attacker_reqs_count;
    attacker_params.io_chunk_size = bmark.io_chunk_size;

    gdriver_ectx_conf_t attacker_conf;
    memset((void *)&attacker_conf, 0, sizeof(gdriver_ectx_conf_t));
    attacker_conf.handler.file_name = bmark.handler_file;
    attacker_conf.handler.hh_name = bmark.hh;
    attacker_conf.handler.ph_name = bmark.ph;
    attacker_conf.handler.th_name = bmark.th;

    char attacker_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.1";
    attacker_conf.fmq_matching_rule.ptr = attacker_addr;
    attacker_conf.fmq_matching_rule.size = strlen(attacker_addr) + 1;
    attacker_conf.slo.compute_prio = 0;

    attacker_ectx_id = gdriver_add_ectx(&attacker_conf);
    if (attacker_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    gdriver_packet_filler_t attacker_pkt_filler;
    memset((void *)&attacker_pkt_filler, 0, sizeof(gdriver_packet_filler_t));
    attacker_pkt_filler.cb = fill_benchmark_params_cb;
    attacker_pkt_filler.data = &attacker_params;
    attacker_pkt_filler.size = sizeof(io_params_t);
    gdriver_add_pkt_filler(attacker_ectx_id, &attacker_pkt_filler);

    if (gdriver_run() != GDRIVER_OK)
        return EXIT_FAILURE;

    return (gdriver_fini() == GDRIVER_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
