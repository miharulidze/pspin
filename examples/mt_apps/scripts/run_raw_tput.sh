#!/bin/bash

TRACES_DIR=$1
LOGS_DIR=$2

for TRACE_PREFIX in {"t1.100.64b","t1.100.512b","t1.100.1024b","t1.100.2048b","t1.100.4096b"}
do
    for KERNEL in {"filtering_ph","histogram_l1_ph","aggregate_global_ph","io_write_ph","io_read_ph","reduce_l1_ph"}
    do
        CMD="FMQ_ARBITER_TYPE=RR ./sim_raw_tput -t ${TRACES_DIR}/${TRACE_PREFIX}.trace ${KERNEL} &> ${LOGS_DIR}/raw_tput.baseline.${KERNEL}.${TRACE_PREFIX}.log"
        echo "$CMD"
        eval $CMD

        CMD="FMQ_ARBITER_TYPE=WLBVT OSMOSIS_EGRESS_FRAGMENTATION=1 OSMOSIS_EGRESS_FSIZE=512 ./sim_raw_tput -t ${TRACES_DIR}/${TRACE_PREFIX}.trace osmosis_${KERNEL} &> ${LOGS_DIR}/raw_tput.osmosis.${KERNEL}.${TRACE_PREFIX}.log"
        echo "$CMD"
        eval $CMD
    done
done
