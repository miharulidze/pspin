#!/bin/bash

LOGS_DIR=$1
TRACES_DIR=$2

VICTIM_SIZE=64
VICTIM_COUNT=1
ATTACKER_COUNT=16

for BMARK in {"dma_l2_read_ph","dma_to_host_ph","dma_from_host_ph","send_packet_ph"}
do
    for ATTACKER_SIZE in {64,128,256,512,1024,2048,4096}
    do
        for TRACE_PREFIX in {"congested","no_congestion"}
        do
            CMD="./sim_io_contention -t ${TRACES_DIR}/${TRACE_PREFIX}.trace ${BMARK} ${VICTIM_SIZE} ${VICTIM_COUNT} ${ATTACKER_SIZE} ${ATTACKER_COUNT} 42 &> ${LOGS_DIR}/2.${BMARK}.victim${VICTIM_SIZE}.atacker${ATTACKER_SIZE}.baseline.${TRACE_PREFIX}.log"
            echo "$CMD"
            #eval $CMD
        done
    done
done

for BMARK in {"osmosis_dma_l2_read_ph","osmosis_dma_to_host_ph","osmosis_dma_from_host_ph","osmosis_send_packet_ph"}
do
    for ATTACKER_SIZE in {64,128,256,512,1024,2048,4096}
    do
        for CHUNK_SIZE in {64,128,256,512,1024}
        do
            for TRACE_PREFIX in {"congested","no_congestion"}
            do
                CMD="./sim_io_contention -t ${TRACES_DIR}/${TRACE_PREFIX}.trace ${BMARK} ${VICTIM_SIZE} ${VICTIM_COUNT} ${ATTACKER_SIZE} ${ATTACKER_COUNT} ${CHUNK_SIZE} &> ${LOGS_DIR}/2.${BMARK}.victim${VICTIM_SIZE}.atacker${ATTACKER_SIZE}.c${CHUNK_SIZE}.${TRACE_PREFIX}.log"
                echo "$CMD"
                #eval $CMD
            done
        done
    done
done
