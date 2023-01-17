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
#include "../handlers/synthetic.h"

int main(int argc, char **argv)
{
    const char *handlers_file = "build/synthetic";
    const char *hh = NULL;
    const char *ph = "synthetic_ph";
    const char *th = NULL;
    benchmark_params_t params[2];
    int ectx_num, ret;

    /* Tenant 1 */
    params[0].loop_spin_count = 0;
    params[0].dma_to_size = 0;
    params[0].dma_to_count = 0;
    params[0].dma_from_size = 0;
    params[0].dma_from_count = 16;

    /* Tenant 2 */
    params[1].loop_spin_count = 0;
    params[1].dma_to_size = 0;
    params[1].dma_to_count = 0;
    params[1].dma_from_size = 0;
    params[1].dma_from_count = 1;

    if (gdriver_init(argc, argv, NULL, &ectx_num) != GDRIVER_OK)
        return EXIT_FAILURE;

    for (int ectx_id = 0; ectx_id < ectx_num; ectx_id++) {
        ret = gdriver_add_ectx(handlers_file, hh, ph, th, NULL,
                               (void *)&params[1], sizeof(params[1]),
                               NULL, 0);
        if (ret != GDRIVER_OK) {
            return EXIT_FAILURE;
        }
    }

    if (gdriver_run() != GDRIVER_OK)
        return EXIT_FAILURE;

    return (gdriver_fini() == GDRIVER_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
