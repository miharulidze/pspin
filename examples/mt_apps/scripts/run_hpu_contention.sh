#!/bin/bash

TRACES_DIR=$1
LOGS_DIR=$2

VICTIM_ITERS=256
VICTIM_PRIO=0
ATTACKER_ITERS=512
ATTACKER_PRIO=0

for TRACE_PREFIX in {"t2mix100.128b","t2mix300.128b","t2mix100","t2mix300"}
do
    for ARBITER in {"RR","BVT"}
    do
    CMD="FMQ_ARBITER_TYPE=${ARBITER} ./sim_hpu_contention -t ${TRACES_DIR}/${TRACE_PREFIX}.trace ${VICTIM_ITERS} ${VICTIM_PRIO} ${ATTACKER_ITERS} ${ATTACKER_PRIO} &> ${LOGS_DIR}/hpu_contention.v${VICTIM_ITERS}.a${ATTACKER_ITERS}.${ARBITER}.${TRACE_PREFIX}.log"
    echo "$CMD"
    eval $CMD
    done
done
