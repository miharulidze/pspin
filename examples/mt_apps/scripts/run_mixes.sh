#!/bin/bash

TRACES_DIR=$1
LOGS_DIR=$2

for TRACE_PREFIX in {"compute.500.0","compute.1000.0"}
do
    for MIX in "compute"
    do
        CMD="FMQ_ARBITER_TYPE=WRR ./sim_${MIX}_mix -t ${TRACES_DIR}/${TRACE_PREFIX}.trace baseline &> ${LOGS_DIR}/baseline.${TRACE_PREFIX}.log"
        echo "$CMD"
        eval $CMD

        CMD="FMQ_ARBITER_TYPE=WLBVT OSMOSIS_EGRESS_FRAGMENTATION=1 OSMOSIS_EGRESS_FSIZE=512 ./sim_${MIX}_mix -t ${TRACES_DIR}/${TRACE_PREFIX}.trace osmosis &> ${LOGS_DIR}/osmosis.${TRACE_PREFIX}.log"
        echo "$CMD"
        eval $CMD
    done
done

for TRACE_PREFIX in {"io.500.0","io.1000.0"}
do
    for MIX in "io"
    do
        CMD="FMQ_ARBITER_TYPE=WRR ./sim_${MIX}_mix -t ${TRACES_DIR}/${TRACE_PREFIX}.trace baseline &> ${LOGS_DIR}/baseline.${TRACE_PREFIX}.log"
        echo "$CMD"
        eval $CMD

        CMD="FMQ_ARBITER_TYPE=WLBVT OSMOSIS_EGRESS_FRAGMENTATION=1 OSMOSIS_EGRESS_FSIZE=512 ./sim_${MIX}_mix -t ${TRACES_DIR}/${TRACE_PREFIX}.trace osmosis &> ${LOGS_DIR}/osmosis.${TRACE_PREFIX}.log"
        echo "$CMD"
        eval $CMD
    done
done
