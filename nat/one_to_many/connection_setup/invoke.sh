#!/bin/bash
mkdir -p out/
SERVER_IP=3.120.108.197

consumers=(1 2 4 8 16 32 64 128)

for consumer in "${consumers[@]}"
    do
    mkdir -p "out/$consumer"
    for run in {1..10}
        do
            timestamp=$(date +%s)
            aws lambda invoke --function-name natbenchmarksetup --cli-read-timeout 180 --cli-connect-timeout 180 --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"$timestamp"'", "role": "producer", "numConsumers":'"$consumer"' }' "out/$consumer/producer_${run}.json" &
            for consumernum in $(seq 1 $consumer);
                do
                aws lambda invoke --function-name natbenchmarksetup --cli-read-timeout 180 --cli-connect-timeout 180 --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"${timestamp}_${consumernum}"'", "role": "consumer" }' "out/$consumer/consumer_${run}_${consumernum}.json" &
                done
            if [ "$consumer" -gt "100" ]; then
                sleep 120
            else
                sleep 20
            fi
        done
    done