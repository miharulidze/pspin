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

#include "gdriver.h"
#include "packets.h"
#include "../handlers/synthetic.h"

int match_ectx_cb(char *arg1, char *arg2)
{
    const char *src_addr = arg1; // arg1 is source address of packet in trace
    const char *ectx_match_addr = arg2;

    printf("DEBUG: %s %s\n", src_addr, ectx_match_addr);

    if (!strcmp(src_addr, ectx_match_addr)) {
        return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    const char *handlers_file = "build/synthetic";
    const char *hh = NULL;
    const char *ph = "synthetic_ph";
    const char *th = NULL;
    benchmark_params_t params[2];
    int ectx_num, ret;

    const char *base_addr = "192.168.0.";
    char matching_addr[GDRIVER_MATCHING_CTX_MAXSIZE];

    /* Tenant 1: Victim */
    params[0].loop_spin_count = 256;
    params[0].dma_to_size = 0;
    params[0].dma_to_count = 0;
    params[0].dma_from_size = 0;
    params[0].dma_from_count = 0;

    /* Tenant 2: Aggressor */
    params[1].loop_spin_count = 512;
    params[1].dma_to_size = 0;
    params[1].dma_to_count = 0;
    params[1].dma_from_size = 0;
    params[1].dma_from_count = 0;

    if (gdriver_init(argc, argv, match_ectx_cb, &ectx_num) != GDRIVER_OK)
        return EXIT_FAILURE;

    for (int ectx_id = 0; ectx_id < ectx_num; ectx_id++) {
        sprintf(matching_addr, "%s%d", base_addr, ectx_id + 1);
        printf("%s\n", matching_addr);
        ret = gdriver_add_ectx(handlers_file, hh, ph, th, NULL,
                               (void *)&params[ectx_id], sizeof(params[ectx_id]),
                               matching_addr, GDRIVER_MATCHING_CTX_MAXSIZE);
        if (ret != GDRIVER_OK) {
            return EXIT_FAILURE;
        }
    }

    if (gdriver_run() != GDRIVER_OK)
        return EXIT_FAILURE;

    return (gdriver_fini() == GDRIVER_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
