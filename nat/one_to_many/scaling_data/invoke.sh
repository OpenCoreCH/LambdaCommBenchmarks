#!/bin/bash
mkdir -p out/

SERVER_IP=18.184.154.126
filesizes=(1 1000 10000 100000 1000000 10000000 100000000 500000000)
consumer=8

for filesize in "${filesizes[@]}"
do
    for run in {1..10}
    do
        timestamp=$(date +%s)
        aws lambda invoke --function-name natbenchmark --cli-read-timeout 120 --cli-connect-timeout 120 --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"', "numConsumers":'"$consumer"' }' "out/producer_${filesize}_${run}.json" &
        for consumernum in $(seq 1 $consumer);
            do
            aws lambda invoke --function-name natbenchmark --cli-read-timeout 120 --cli-connect-timeout 120 --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"${timestamp}_${consumernum}"'", "role": "consumer", "fileSize": '"$filesize"' }' "out/consumer_${filesize}_${run}_${consumernum}.json" &
            done
        if [ "$filesize" -gt "100000000" ]; then
            sleep 125
        else
            sleep 10
        fi
    done
done
