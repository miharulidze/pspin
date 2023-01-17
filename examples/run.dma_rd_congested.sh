#!/bin/bash

cd $1
for i in 64 256 1024 2048 4096
do
    ./sim_${1} -m 2 -p 8 -s $i &> sim_congested_dma_rd.${i}.log
done
