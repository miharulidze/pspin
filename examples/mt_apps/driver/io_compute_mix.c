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
    int ectx_num;
    int ra_ectx_id, fv_ectx_id, ha_ectx_id, iowa_ectx_id;
    int use_osmosis;

    assert(argc == 4);

    if (!strcmp(argv[3], "baseline")) {
        use_osmosis = 0;
    } else if (!strcmp(argv[3], "osmosis")) {
        use_osmosis = 1;
    } else {
        assert(0);
    }

    assert(gdriver_init(argc, argv, match_ectx_cb, &ectx_num) == GDRIVER_OK);

    /* Reduce 4K attacker (ra) 2P --> (ectx #0) --> 192.168.0.0 */
    /* Filtering victim (fv) --> (ectx #1) --> 192.168.0.1 */
    /* Historgram attacker (ha) --> (ectx #2) --> 192.168.0.2 */
    /* Iow attacker (iowa) --> (ectx #3) --> 192.168.0.3 */

    const char *reduce_ph = use_osmosis ? "osmosis_reduce_l1_ph" : "reduce_l1_ph";
    const char *hist_ph = use_osmosis ? "osmosis_histogram_l1_ph" : "histogram_l1_ph";
    const char *iowr_ph = use_osmosis ? "osmosis_io_write_ph" : "io_write_ph";
    const char *filtering_ph = use_osmosis ? "osmosis_filtering_ph" : "filtering_ph";

    /* ra */
    gdriver_ectx_conf_t ra_conf;
    memset((void *)&ra_conf, 0, sizeof(gdriver_ectx_conf_t));
    ra_conf.handler.file_name = handler_file;
    ra_conf.handler.hh_name = NULL;
    ra_conf.handler.ph_name = reduce_ph;
    ra_conf.handler.th_name = NULL;

    char ra_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.0";
    ra_conf.fmq_matching_rule.ptr = ra_addr;
    ra_conf.fmq_matching_rule.size = strlen(ra_addr) + 1;
    ra_conf.slo.compute_prio = 2;

    ra_ectx_id = gdriver_add_ectx(&ra_conf);
    if (ra_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    /* fv */
    gdriver_ectx_conf_t fv_conf;
    memset((void *)&fv_conf, 0, sizeof(gdriver_ectx_conf_t));
    fv_conf.handler.file_name = handler_file;
    fv_conf.handler.hh_name = NULL;
    fv_conf.handler.ph_name = filtering_ph;
    fv_conf.handler.th_name = NULL;

    char fv_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.1";
    fv_conf.fmq_matching_rule.ptr = fv_addr;
    fv_conf.fmq_matching_rule.size = strlen(fv_addr) + 1;
    fv_conf.slo.compute_prio = 0;

    srand(SEED);
    uint32_t *vec = (uint32_t *)malloc(sizeof(uint32_t) * (TOT_WORDS / ectx_num));
    fill_htable(vec, TOT_WORDS / ectx_num);
    fv_conf.handler_l2_img.ptr = vec;
    fv_conf.handler_l2_img.size = sizeof(uint32_t) * TOT_WORDS / ectx_num;

    fv_ectx_id = gdriver_add_ectx(&fv_conf);
    if (fv_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    /* ha */
    gdriver_packet_filler_t hist_pkt_filler;
    memset((void *)&hist_pkt_filler, 0, sizeof(gdriver_packet_filler_t));
    hist_pkt_filler.cb = histogram_l1_fill_cb;
    hist_pkt_filler.data = NULL;
    hist_pkt_filler.size = 0;

    gdriver_ectx_conf_t ha_conf;
    memset((void *)&ha_conf, 0, sizeof(gdriver_ectx_conf_t));
    ha_conf.handler.file_name = handler_file;
    ha_conf.handler.hh_name = NULL;
    ha_conf.handler.ph_name = hist_ph;
    ha_conf.handler.th_name = NULL;

    char ha_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.2";
    ha_conf.fmq_matching_rule.ptr = ha_addr;
    ha_conf.fmq_matching_rule.size = strlen(ha_addr) + 1;
    ha_conf.slo.compute_prio = 1;

    ha_ectx_id = gdriver_add_ectx(&ha_conf);
    if (ha_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    gdriver_add_pkt_filler(ha_ectx_id, &hist_pkt_filler);

    /* iowa */
    gdriver_ectx_conf_t iowa_conf;
    memset((void *)&iowa_conf, 0, sizeof(gdriver_ectx_conf_t));
    iowa_conf.handler.file_name = handler_file;
    iowa_conf.handler.hh_name = NULL;
    iowa_conf.handler.ph_name = iowr_ph;
    iowa_conf.handler.th_name = NULL;

    char iowa_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.3";
    iowa_conf.fmq_matching_rule.ptr = iowa_addr;
    iowa_conf.fmq_matching_rule.size = strlen(iowa_addr) + 1;
    iowa_conf.slo.compute_prio = 0;

    iowa_ectx_id = gdriver_add_ectx(&iowa_conf);
    if (iowa_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    if (gdriver_run() != GDRIVER_OK)
        return EXIT_FAILURE;

    return (gdriver_fini() == GDRIVER_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
