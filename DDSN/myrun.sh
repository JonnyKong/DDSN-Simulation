#!/bin/bash

set -e  # Exit on error

RUN_TIMES=12
NODE_NUM=20
LOSS_RATE_LIST=(0.0 0.05 0.2 0.5)

# WIFI_RANGE_LIST=(60 80 100 120 140 160)
WIFI_RANGE_LIST=(20 30 40 50 60 70 80 90 100)
# WIFI_RANGE_LIST=(60)

run_loss_rate() {
    local LOSS_RATE=$1
    local RESULT_DIR=$2
    echo "Starting simulation: Loss rate = ${LOSS_RATE} ..."
    NS_LOG='SyncForSleep' ./waf --run "sync-for-sleep-movepattern --pauseTime=0 \
        --run=0 --mobileNodeNum=${NODE_NUM} --lossRate=${LOSS_RATE} --wifiRange=60" \
        > ${RESULT_DIR}/raw/loss_rate_${LOSS_RATE}.txt 2>&1
    python syncDuration.py ${RESULT_DIR}/raw/loss_rate_${LOSS_RATE}.txt ${NODE_NUM} \
        > ${RESULT_DIR}/loss_rate_${LOSS_RATE}.txt
}

run_wifi_range() {
    local WIFI_RANGE=$1
    local RESULT_DIR=$2
    echo "Starting simulation: Wifi range = ${WIFI_RANGE} ..."
    NS_LOG='SyncForSleep' ./waf --run "sync-for-sleep-movepattern --pauseTime=0 \
        --run=0 --mobileNodeNum=${NODE_NUM} --lossRate=0 --wifiRange=${WIFI_RANGE} "\
        > ${RESULT_DIR}/raw/wifi_range_${WIFI_RANGE}.txt 2>&1
    python syncDuration.py ${RESULT_DIR}/raw/wifi_range_${WIFI_RANGE}.txt ${NODE_NUM} \
        > ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt
}

summarize_wifi_range_result() {
    local RESULT_DIR=$1
    local FILENAME=wifi_range.txt

    # Sync interest
    echo "Sync interest:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "out notify interest" >> ${RESULT_DIR}/${FILENAME}
    done

    # Sync ack
    echo "Sync ack:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "out ack" >> ${RESULT_DIR}/${FILENAME}
    done

    # Data interest
    echo "Data interest:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "out data interest" >> ${RESULT_DIR}/${FILENAME}
    done

    # Data reply
    echo "Data reply:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "data reply" \
            >> ${RESULT_DIR}/${FILENAME}
            # | grep "out data" \
            # | grep -v "out data interest" \
    done

    # # Bundled interest
    # echo "Bundled interest:" >> ${RESULT_DIR}/${FILENAME}
    # for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
    #     echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
    #     cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
    #         | grep "out bundled interest" >> ${RESULT_DIR}/${FILENAME}
    # done

    # # Bundled data
    # echo "Bundled data:" >> ${RESULT_DIR}/${FILENAME}
    # for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
    #     echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
    #     cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
    #         | grep "out bundled data" >> ${RESULT_DIR}/${FILENAME}
    # done

    # # Transmitted data (out data + out ack + out bundled data)
    # echo "Transmitted data (number of packets):" >> ${RESULT_DIR}/${FILENAME}
    # for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
    #     echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
    #     cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
    #         | grep "out" \
    #         | grep "data\|ack\|bundled data" \
    #         | grep -v "interest" \
    #         | awk '{sum+=$NF} END {print sum}' \
    #         >> ${RESULT_DIR}/${FILENAME}
    # done

    # State sync delay
    echo "State sync delay:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "state sync delay" >> ${RESULT_DIR}/${FILENAME}
    done

    # Data sync delay
    echo "Data sync delay:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "data sync delay" >> ${RESULT_DIR}/${FILENAME}
    done

    # Recv data
    echo "Recvd Data Reply:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "recvDataReply" >> ${RESULT_DIR}/${FILENAME}
    done

    # Forwarded data
    echo "Forwarded Data Reply:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "recvForwardedDataReply" >> ${RESULT_DIR}/${FILENAME}
    done

    # Suppressed data
    echo "Suppressed Data Reply:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "recvSuppressedDataReply" >> ${RESULT_DIR}/${FILENAME}
    done

    # # Number of collisions
    # echo "Number of collisions: " >> ${RESULT_DIR}/${FILENAME}
    # for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
    #     echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
    #     cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
    #         | grep "number of collision" >> ${RESULT_DIR}/${FILENAME}
    # done

    # Number of received sync interests
    echo "Received sync interest:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "in notify interest" >> ${RESULT_DIR}/${FILENAME}
    done

    # Number of suppressed sync interests
    echo "Suppressed sync interest:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "suppressed notify interest" >> ${RESULT_DIR}/${FILENAME}
    done

    # Rate of collision
    echo "Collision rate:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "collision rate" >> ${RESULT_DIR}/${FILENAME}
    done

    # Repo reply rate
    echo "Repo reply rate:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "repo reply rate" >> ${RESULT_DIR}/${FILENAME}
    done

    # Hibernate duration
    echo "Hibernate duration:" >> ${RESULT_DIR}/${FILENAME}
    for WIFI_RANGE in "${WIFI_RANGE_LIST[@]}"; do
        echo -n "Wifi range = ${WIFI_RANGE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/wifi_range_${WIFI_RANGE}.txt \
            | grep "hibernate duration" >> ${RESULT_DIR}/${FILENAME}
    done
}

summarize_loss_rate_result() {
    local RESULT_DIR=$1
    local FILENAME=loss_rate.txt

    # Sync interest
    echo "Sync interest:" >> ${RESULT_DIR}/${FILENAME}
    for LOSS_RATE in "${LOSS_RATE_LIST[@]}"; do
        echo -n "Loss rate = ${LOSS_RATE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/loss_rate_${LOSS_RATE}.txt \
            | grep "out notify interest" >> ${RESULT_DIR}/${FILENAME}
    done

    # Sync ack
    echo "Sync ack:" >> ${RESULT_DIR}/${FILENAME}
    for LOSS_RATE in "${LOSS_RATE_LIST[@]}"; do
        echo -n "Loss rate = ${LOSS_RATE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/loss_rate_${LOSS_RATE}.txt \
            | grep "out ack" >> ${RESULT_DIR}/${FILENAME}
    done

    # Data interest
    echo "Data interest:" >> ${RESULT_DIR}/${FILENAME}
    for LOSS_RATE in "${LOSS_RATE_LIST[@]}"; do
        echo -n "Loss rate = ${LOSS_RATE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/loss_rate_${LOSS_RATE}.txt \
            | grep "out data interest" >> ${RESULT_DIR}/${FILENAME}
    done

    # Data reply
    echo "Data reply:" >> ${RESULT_DIR}/${FILENAME}
    for LOSS_RATE in "${LOSS_RATE_LIST[@]}"; do
        echo -n "Loss rate = ${LOSS_RATE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/loss_rate_${LOSS_RATE}.txt \
            | grep "data reply" \
            >> ${RESULT_DIR}/${FILENAME}
    done

    # State sync delay
    echo "State sync delay:" >> ${RESULT_DIR}/${FILENAME}
    for LOSS_RATE in "${LOSS_RATE_LIST[@]}"; do
        echo -n "Loss rate = ${LOSS_RATE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/loss_rate_${LOSS_RATE}.txt \
            | grep "state sync delay" >> ${RESULT_DIR}/${FILENAME}
    done

    # Data sync delay
    echo "Data sync delay:" >> ${RESULT_DIR}/${FILENAME}
    for LOSS_RATE in "${LOSS_RATE_LIST[@]}"; do
        echo -n "Loss rate = ${LOSS_RATE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/loss_rate_${LOSS_RATE}.txt \
            | grep "data sync delay" >> ${RESULT_DIR}/${FILENAME}
    done

    # Rate of collision
    echo "Collision rate:" >> ${RESULT_DIR}/${FILENAME}
    for LOSS_RATE in "${LOSS_RATE_LIST[@]}"; do
        echo -n "Loss rate = ${LOSS_RATE} " >> ${RESULT_DIR}/${FILENAME}
        cat ${RESULT_DIR}/loss_rate_${LOSS_RATE}.txt \
            | grep "collision rate" >> ${RESULT_DIR}/${FILENAME}
    done
}

main() {
    # local RESULT_DIR=result/$(date -I)
    local RESULT_DIR=result
    rm -rf $RESULT_DIR

    # # Run different wifi ranges
    # for (( TIME=1; TIME<=$RUN_TIMES; TIME++ )); do
    #     mkdir -p ${RESULT_DIR}/${TIME}/raw
    #     local pids=""
    #     for i in "${WIFI_RANGE_LIST[@]}"; do
    #         run_wifi_range $i ${RESULT_DIR}/${TIME} &
    #         pids="$pids $!"
    #         sleep 10
    #     done
    #     wait $pids
    #     summarize_wifi_range_result ${RESULT_DIR}/${TIME}
    # done
    # for (( TIME=1; TIME<=$RUN_TIMES; TIME++ )); do
    #     mv ${RESULT_DIR}/${TIME}/wifi_range.txt ${RESULT_DIR}/wifi_range_${TIME}.txt
    # done
    # python calculate_mean.py WIFI_RANGE ${RUN_TIMES} >> $RESULT_DIR/wifi_range.txt

    # # Run different loss rates
    # for (( TIME=1; TIME<=$RUN_TIMES; TIME++ )); do
    #     mkdir -p ${RESULT_DIR}/${TIME}/raw
    #     local pids=""
    #     for i in "${LOSS_RATE_LIST[@]}"; do
    #         run_loss_rate $i ${RESULT_DIR}/${TIME} &
    #         pids="$pids $!"
    #         sleep 10
    #     done
    #     wait $pids
    #     summarize_loss_rate_result ${RESULT_DIR}/${TIME}
    # done
    # for (( TIME=1; TIME<=$RUN_TIMES; TIME++ )); do
    #     mv ${RESULT_DIR}/${TIME}/loss_rate.txt ${RESULT_DIR}/loss_rate_${TIME}.txt
    # done
    # python calculate_mean.py LOSS_RATE ${RUN_TIMES} >> $RESULT_DIR/loss_rate.txt

    ./waf
    for i in "${LOSS_RATE_LIST[@]}"; do
        local pids=""
        for (( TIME=1; TIME<=$RUN_TIMES; TIME++ )); do
            mkdir -p ${RESULT_DIR}/${TIME}/raw
            run_loss_rate $i ${RESULT_DIR}/${TIME} &
            sleep 0.1
            pids="$pids $!"
        done
        wait $pids
    done

    for (( TIME=1; TIME<=$RUN_TIMES; TIME++ )); do
        summarize_loss_rate_result ${RESULT_DIR}/${TIME}
        mv ${RESULT_DIR}/${TIME}/loss_rate.txt ${RESULT_DIR}/loss_rate_${TIME}.txt
    done
    python calculate_mean.py LOSS_RATE ${RUN_TIMES} >> $RESULT_DIR/loss_rate.txt
}

main
