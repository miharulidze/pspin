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
    int iorv_ectx_id, iora_ectx_id, iowrv_ectx_id, iowra_ectx_id;
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

    const char *ior_ph = use_osmosis ? "osmosis_io_read_ph" : "io_read_ph";
    const char *iowr_ph = use_osmosis ? "osmosis_io_write_ph" : "io_write_ph";

    /* io_read victim (iorv) --> (ectx #0) --> 192.168.0.0 */
    /* io_write victim (iowrv) --> (ectx #1) --> 192.168.0.1 */
    /* io_read attacker (iora) --> (ectx #2) --> 192.168.0.2 */
    /* io_write attacker (iowra) --> (ectx #3) --> 192.168.0.3 */

    /* iorv */
    gdriver_ectx_conf_t iorv_conf;
    memset((void *)&iorv_conf, 0, sizeof(gdriver_ectx_conf_t));
    iorv_conf.handler.file_name = handler_file;
    iorv_conf.handler.hh_name = NULL;
    iorv_conf.handler.ph_name = ior_ph;
    iorv_conf.handler.th_name = NULL;

    char iorv_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.0";
    iorv_conf.fmq_matching_rule.ptr = iorv_addr;
    iorv_conf.fmq_matching_rule.size = strlen(iorv_addr) + 1;
    iorv_conf.slo.compute_prio = 0;

    iorv_ectx_id = gdriver_add_ectx(&iorv_conf);
    if (iorv_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    /* iowrv */
    gdriver_ectx_conf_t iowrv_conf;
    memset((void *)&iowrv_conf, 0, sizeof(gdriver_ectx_conf_t));
    iowrv_conf.handler.file_name = handler_file;
    iowrv_conf.handler.hh_name = NULL;
    iowrv_conf.handler.ph_name = iowr_ph;
    iowrv_conf.handler.th_name = NULL;

    char iowrv_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.1";
    iowrv_conf.fmq_matching_rule.ptr = iowrv_addr;
    iowrv_conf.fmq_matching_rule.size = strlen(iowrv_addr) + 1;
    iowrv_conf.slo.compute_prio = 0;

    iowrv_ectx_id = gdriver_add_ectx(&iowrv_conf);
    if (iowrv_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    /* iora */
    gdriver_ectx_conf_t iora_conf;
    memset((void *)&iora_conf, 0, sizeof(gdriver_ectx_conf_t));
    iora_conf.handler.file_name = handler_file;
    iora_conf.handler.hh_name = NULL;
    iora_conf.handler.ph_name = ior_ph;
    iora_conf.handler.th_name = NULL;

    char iora_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.2";
    iora_conf.fmq_matching_rule.ptr = iora_addr;
    iora_conf.fmq_matching_rule.size = strlen(iora_addr) + 1;
    iora_conf.slo.compute_prio = 0;

    iora_ectx_id = gdriver_add_ectx(&iora_conf);
    if (iora_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    /* iowra */
    gdriver_ectx_conf_t iowra_conf;
    memset((void *)&iowra_conf, 0, sizeof(gdriver_ectx_conf_t));
    iowra_conf.handler.file_name = handler_file;
    iowra_conf.handler.hh_name = NULL;
    iowra_conf.handler.ph_name = iowr_ph;
    iowra_conf.handler.th_name = NULL;

    char iowra_addr[GDRIVER_FMQ_MATCHING_RULE_MAXSIZE] = "192.168.0.3";
    iowra_conf.fmq_matching_rule.ptr = iowra_addr;
    iowra_conf.fmq_matching_rule.size = strlen(iowra_addr) + 1;
    iowra_conf.slo.compute_prio = 0;

    iowra_ectx_id = gdriver_add_ectx(&iowra_conf);
    if (iowra_ectx_id == GDRIVER_ERR)
        return EXIT_FAILURE;

    if (gdriver_run() != GDRIVER_OK)
        return EXIT_FAILURE;

    return (gdriver_fini() == GDRIVER_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
