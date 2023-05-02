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

#define SEED 42
#define MAX_STRING_LEN 1024
const char *handler_file = "build/kernels";
//const char *hh = NULL;
//const char *ph = "spinner_ph";
//const char *th = NULL;

int bmark_parse_arguments(int argc, char **argv, const char **ph)
{
    if (argc != 4) {
        printf("bad arguments: ./sim_hpu_contention -t <trace_name> <kernel_name>\n");
        assert(0);
    }

    *ph = argv[3];

    printf("Kernel=%s\n", *ph);

    return GDRIVER_OK;
}

uint16_t hashmult(const uint16_t key)
{
    uint16_t hash = 0;
    uint8_t *key_byte = (uint8_t *)&key;
    for (uint16_t i = 0; i < sizeof(uint16_t); i++)
        hash = hash * 31 + (key_byte[i]);

    return hash;
}

void fill_htable(uint32_t *vec, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++) {
        vec[hashmult((uint16_t)i)] = i;
    }
}

int filtering_fill_cb(void *pkt, size_t pkt_size, void *params)
{
    for (int i = 0; i < KEY_SIZE / sizeof(uint32_t); i++)
        ((uint32_t *)pkt)[i] = rand();

    return 0;
}

int histogram_l1_fill_cb(void *pkt, size_t pkt_size, void *params)
{
    pkt_hdr_t *hdr = (pkt_hdr_t*) pkt;
    uint32_t payload_len = pkt_size - sizeof(pkt_hdr_t);
    uint32_t *payload_ptr = (uint32_t *)(hdr + 1);

    for (int i=0; i < payload_len / sizeof(uint32_t); i++)
        payload_ptr[i] = rand() % HISTOGRAM_WORDS;

    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    int ectx_num, kernel_ectx_id;
    const char *ph;

    assert(bmark_parse_arguments(argc, argv, &ph) == GDRIVER_OK);
    assert(gdriver_init(argc, argv, match_ectx_cb, &ectx_num) == GDRIVER_OK);

    //assert(ectx_num == 1);

    /* kernel --> (ectx #0) --> 192.168.0.0 */

    gdriver_ectx_conf_t kernel_conf;
    memset((void *)&kernel_conf, 0, sizeof(gdriver_ectx_conf_t));
    kernel_conf.handler.file_name = handler_file;
    kernel_conf.handler.hh_name = NULL;
    kernel_conf.handler.ph_name = ph;
    kernel_conf.handler.th_name = NULL;
    char kernel_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.0";
    kernel_conf.fmq_matching_rule.ptr = kernel_addr;
    kernel_conf.fmq_matching_rule.size = strlen(kernel_addr) + 1;
    kernel_conf.slo.compute_prio = 0;

    if (!strcmp(kernel_conf.handler.ph_name, "filtering_ph")) {
        srand(SEED);
        uint32_t *vec = (uint32_t *)malloc(sizeof(uint32_t) * (TOT_WORDS / ectx_num));
        fill_htable(vec, TOT_WORDS / ectx_num);
        kernel_conf.handler_l2_img.ptr = vec;
        kernel_conf.handler_l2_img.size = sizeof(uint32_t) * TOT_WORDS / ectx_num;
    }

    kernel_ectx_id = gdriver_add_ectx(&kernel_conf);
    if (kernel_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    gdriver_packet_filler_t kernel_pkt_filler;
    if (!strcmp(kernel_conf.handler.ph_name, "filtering_ph") ||
        !strcmp(kernel_conf.handler.ph_name, "osmosis_filtering_ph")) {
        kernel_pkt_filler.cb = filtering_fill_cb;
        kernel_pkt_filler.data = NULL;
        kernel_pkt_filler.size = 0;
        gdriver_add_pkt_filler(kernel_ectx_id, &kernel_pkt_filler);
    } else if (!strcmp(kernel_conf.handler.ph_name, "histogram_l1_ph") ||
               !strcmp(kernel_conf.handler.ph_name, "osmosis_histogram_l1_ph")) {
        kernel_pkt_filler.cb = histogram_l1_fill_cb;
        kernel_pkt_filler.data = NULL;
        kernel_pkt_filler.size = 0;
        gdriver_add_pkt_filler(kernel_ectx_id, &kernel_pkt_filler);
    } else {
        gdriver_add_pkt_filler(kernel_ectx_id, NULL);
    }

    if (gdriver_run() != GDRIVER_OK)
        return EXIT_FAILURE;

    free(kernel_conf.handler_l2_img.ptr);
    return (gdriver_fini() == GDRIVER_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
