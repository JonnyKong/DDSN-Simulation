#!/bin/bash

set -e  # Exit on error

RUN_TIMES=12
NODE_NUM=20
LOSS_RATE_LIST=(0.0 0.05 0.2 0.5)
LOSS_RATE_DIR_NAME=(0 5 20 50)

for i in $(seq 0 3); do
    rm -f result/loss_rate_${LOSS_RATE_DIR_NAME[$i]}/*
done

./waf
for i in $(seq 0 3); do
    for (( TIME=1; TIME<=$RUN_TIMES; TIME++ )); do
        echo "Running loss rate = ${LOSS_RATE_LIST[$i]}"
        NS_LOG='SyncForSleep' ./build/sync-for-sleep-movepattern --pauseTime=0 \
            --run=0 --mobileNodeNum=${NODE_NUM} --lossRate=${LOSS_RATE_LIST[$i]} --wifiRange=60 \
            > result/loss_rate_${LOSS_RATE_DIR_NAME[$i]}/result_${TIME}.txt 2>&1 &
        pids="$pids $!"
    done
    wait $pids
done
