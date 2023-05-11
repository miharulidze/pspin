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

#define MAX_STRING_LEN 1024
const char *handler_file = "build/kernels";

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
    int av_ectx_id, aa_ectx_id, hv_ectx_id, ha_ectx_id;
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

    const char *reduce_ph = use_osmosis ? "osmosis_reduce_l1_ph" : "reduce_l1_ph";
    const char *hist_ph = use_osmosis ? "osmosis_histogram_l1_ph" : "histogram_l1_ph";

    /* Reduce victim (av) --> (ectx #0) --> 192.168.0.0 */
    /* Historgram victim (hv) --> (ectx #1) --> 192.168.0.1 */
    /* Reduce attacker (aa) --> (ectx #2) --> 192.168.0.2 */
    /* Histogram attacker (ha) --> (ectx #3) --> 192.168.0.3 */

    /* av */
    gdriver_ectx_conf_t av_conf;
    memset((void *)&av_conf, 0, sizeof(gdriver_ectx_conf_t));
    av_conf.handler.file_name = handler_file;
    av_conf.handler.hh_name = NULL;
    av_conf.handler.ph_name = reduce_ph;
    av_conf.handler.th_name = NULL;

    char av_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.0";
    av_conf.fmq_matching_rule.ptr = av_addr;
    av_conf.fmq_matching_rule.size = strlen(av_addr) + 1;
    av_conf.slo.compute_prio = 0;

    av_ectx_id = gdriver_add_ectx(&av_conf);
    if (av_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    /* hv */
    gdriver_packet_filler_t hist_pkt_filler;
    memset((void *)&hist_pkt_filler, 0, sizeof(gdriver_packet_filler_t));
    hist_pkt_filler.cb = histogram_l1_fill_cb;
    hist_pkt_filler.data = NULL;
    hist_pkt_filler.size = 0;

    gdriver_ectx_conf_t hv_conf;
    memset((void *)&hv_conf, 0, sizeof(gdriver_ectx_conf_t));
    hv_conf.handler.file_name = handler_file;
    hv_conf.handler.hh_name = NULL;
    hv_conf.handler.ph_name = hist_ph;
    hv_conf.handler.th_name = NULL;

    char hv_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.1";
    hv_conf.fmq_matching_rule.ptr = hv_addr;
    hv_conf.fmq_matching_rule.size = strlen(hv_addr) + 1;
    hv_conf.slo.compute_prio = 0;

    hv_ectx_id = gdriver_add_ectx(&hv_conf);
    if (hv_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    gdriver_add_pkt_filler(hv_ectx_id, &hist_pkt_filler);

    /* aa */
    gdriver_ectx_conf_t aa_conf;
    memset((void *)&aa_conf, 0, sizeof(gdriver_ectx_conf_t));
    aa_conf.handler.file_name = handler_file;
    aa_conf.handler.hh_name = NULL;
    aa_conf.handler.ph_name = reduce_ph;
    aa_conf.handler.th_name = NULL;

    char aa_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.2";
    aa_conf.fmq_matching_rule.ptr = aa_addr;
    aa_conf.fmq_matching_rule.size = strlen(aa_addr) + 1;
    aa_conf.slo.compute_prio = 0;

    aa_ectx_id = gdriver_add_ectx(&aa_conf);
    if (aa_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    /* ha */
    gdriver_ectx_conf_t ha_conf;
    memset((void *)&ha_conf, 0, sizeof(gdriver_ectx_conf_t));
    ha_conf.handler.file_name = handler_file;
    ha_conf.handler.hh_name = NULL;
    ha_conf.handler.ph_name = hist_ph;
    ha_conf.handler.th_name = NULL;

    char ha_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.3";
    ha_conf.fmq_matching_rule.ptr = ha_addr;
    ha_conf.fmq_matching_rule.size = strlen(ha_addr) + 1;
    ha_conf.slo.compute_prio = 0;

    ha_ectx_id = gdriver_add_ectx(&ha_conf);
    if (ha_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    gdriver_add_pkt_filler(ha_ectx_id, &hist_pkt_filler);

    if (gdriver_run() != GDRIVER_OK)
        return EXIT_FAILURE;

    return (gdriver_fini() == GDRIVER_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
