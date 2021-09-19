#!/bin/bash
mkdir -p out/
REDIS_HOSTNAME=clusterforlambda.2ctj87.0001.euc1.cache.amazonaws.com
REDIS_PORT=6379

filesizes=(1 1000 10000 100000 1000000 10000000 500000000)
filesize=10000000
consumers=(1 2 4 8 16 32 64 128 256)
consumers=(256)

for consumer in "${consumers[@]}"
    do
    mkdir -p "out/$consumer"
    for run in {10..10}
        do
            timestamp=$(date +%s)
            wait_until=$((timestamp + 10))
            aws lambda invoke --function-name redisbenchmark --cli-binary-format raw-in-base64-out --payload '{"hostname":"'"$REDIS_HOSTNAME"'", "port":'"$REDIS_PORT"', "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"', "waitUntil": '"$wait_until"' }' "out/$consumer/producer_${filesize}_${run}.json" &
            for consumernum in $(seq 1 $consumer);
                do
                aws lambda invoke --function-name redisbenchmark --cli-binary-format raw-in-base64-out --payload '{"hostname":"'"$REDIS_HOSTNAME"'", "port":'"$REDIS_PORT"', "key":"'"$timestamp"'", "role": "consumer", "fileSize": '"$filesize"', "waitUntil": '"$wait_until"' }' "out/$consumer/consumer_${filesize}_${run}_${consumernum}.json" &
                done
            if [ "$consumer" -gt "100" ]; then
                sleep 50
            else
                sleep 20
            fi
        done
    done