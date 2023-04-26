#!/bin/bash

TRACES_DIR=$1
VICTIM_SIZE=64
VICTIM_COUNT=1
ATTACKER_COUNT=1

for BMARK in {"dma_l2_read_ph","osmosis_dma_l2_read_ph","dma_to_host_ph","osmosis_dma_to_host_ph","dma_from_host_ph","osmosis_dma_from_host_ph","send_packet_ph","osmosis_send_packet_ph"}
do
    for ATTACKER_SIZE in {64,4096}
    do
        CMD="./sim_io_contention -t ${TRACES_DIR}/test.trace ${BMARK} ${VICTIM_SIZE} ${VICTIM_COUNT} ${ATTACKER_SIZE} ${ATTACKER_COUNT} 64 &> 2.${BMARK}.at${ATTACKER_SIZE}.test.log"
        echo "$CMD"
        eval $CMD
    done
done
