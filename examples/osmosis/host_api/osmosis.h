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

#pragma once

#include <stdint.h>
#include <stdlib.h>

#define GDRIVER_OK 0
#define GDRIVER_ERR -1

#define GDRIVER_FMQ_MATCHING_RULE_MAXSIZE 256

typedef int (*match_packet_fun_t)(void*, void*, void*);

typedef struct gdriver_ectx_conf {
    struct handler {
	const char *file_name;
	const char *hh_name;
	const char *ph_name;
	const char *th_name;
    } handler;

    struct handler_l2_img {
	void *ptr;
	size_t size;
    } handler_l2_img;

    struct fmq_matching_rule {
	void *ptr;
	size_t size;
    } fmq_matching_rule;

    struct slo {
	uint8_t compute_prio;
	uint8_t io_prio;
    } slo;
} gdriver_ectx_conf_t;

typedef struct gdriver_packet_filler {
    int (*cb)(void *, void *);
    void *data;
    size_t size;
} gdriver_packet_filler_t;

int gdriver_init(int argc, char **argv, match_packet_fun_t matching_cb, int *ectx_num);
int gdriver_add_ectx(const gdriver_ectx_conf_t *ectx_conf);
int gdriver_add_pkt_filler(int ectx_id, const gdriver_packet_filler_t *filler);

int gdriver_run();
int gdriver_fini();

