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

#include "gdriver.h"
#include "../handlers/dma.h"

#define MAX_ADDR_LEN 128

int match_ectx_cb(char *src, char *dst, char *ectx_addr)
{
    if (!strcmp(dst, ectx_addr)) {
        printf("DEBUG: src=%s matching_ctx=%s\n", dst, ectx_addr);
        return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    const char *handlers_file = "build/dma";
    const char *hh = NULL;
    const char *ph = "dma_ph";
    const char *th = NULL;
    int ectx_num, ret;
    char victim_addr[MAX_ADDR_LEN] = "192.168.0.0";
    char attacker_addr[MAX_ADDR_LEN] = "192.168.0.1";
    benchmark_params_t victim_params;
    benchmark_params_t attacker_params;

    if (gdriver_init(argc, argv, match_ectx_cb, &ectx_num) != GDRIVER_OK)
        return EXIT_FAILURE;

    /* victim --> (ectx #0) --> 192.168.0.0 */
    victim_params.dma_count = 1;
    victim_params.dma_read_size = 64; // victim generates 1 64B read per each packet
    ret = gdriver_add_ectx(handlers_file, hh, ph, th, NULL,
                           (void *)&victim_params, sizeof(victim_params),
                           victim_addr, strlen(victim_addr) + 1);
    if (ret != GDRIVER_OK)
        return EXIT_FAILURE;

    /* attacker --> (ectx #2) --> 192.168.0.1 */
    attacker_params.dma_count = 8;
    attacker_params.dma_read_size = 1024; // attacker generates 1 1024B read per each packet
    ret = gdriver_add_ectx(handlers_file, hh, ph, th, NULL,
                           (void *)&attacker_params, sizeof(attacker_params),
                           attacker_addr, strlen(attacker_addr) + 1);
    if (ret != GDRIVER_OK)
        return EXIT_FAILURE;

    if (gdriver_run() != GDRIVER_OK)
        return EXIT_FAILURE;

    return (gdriver_fini() == GDRIVER_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
