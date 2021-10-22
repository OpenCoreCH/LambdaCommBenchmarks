#!/bin/bash
mkdir -p out_overhead/
SERVER_IP=52.59.239.255

filesizes=(1 1000 10000 100000 1000000 10000000 500000000)
filesize=1
consumers=(64)

for consumer in "${consumers[@]}"
    do
    mkdir -p "out_overhead/$consumer"
    for run in {11..11}
        do
            timestamp=$(date +%s)
            aws lambda invoke --function-name natbenchmark --cli-read-timeout 180 --cli-connect-timeout 180 --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"', "numConsumers":'"$consumer"' }' "out_overhead/$consumer/producer_${filesize}_${run}.json" &
            for consumernum in $(seq 1 $consumer);
                do
                aws lambda invoke --function-name natbenchmark --cli-read-timeout 180 --cli-connect-timeout 180 --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"${timestamp}_${consumernum}"'", "role": "consumer", "fileSize": '"$filesize"' }' "out_overhead/$consumer/consumer_${filesize}_${run}_${consumernum}.json" &
                done
        sleep 10
        done
    done
