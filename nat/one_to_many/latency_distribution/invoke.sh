#!/bin/bash
mkdir -p out/
SERVER_IP=52.59.227.123

filesizes=(1 1000 10000 100000 1000000 10000000 500000000)
filesize=10000000
consumer=8

for run in {1..100}
    do
        timestamp=$(date +%s)
        aws lambda invoke --function-name natbenchmark --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"', "numConsumers":'"$consumer"' }' "out/producer_${filesize}_${run}.json" &
        for consumernum in $(seq 1 $consumer);
            do
            aws lambda invoke --function-name natbenchmark --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"${timestamp}_${consumernum}"'", "role": "consumer", "fileSize": '"$filesize"' }' "out/consumer_${filesize}_${run}_${consumernum}.json" &
            done
        sleep 10
    done
