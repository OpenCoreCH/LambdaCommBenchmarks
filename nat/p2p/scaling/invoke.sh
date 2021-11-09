#!/bin/bash
mkdir -p out/
SERVER_IP=35.156.180.65

filesizes=(1 1000 10000 100000 1000000 10000000 100000000)

for filesize in "${filesizes[@]}"
do
    for run in {1..10}
    do
        timestamp=$(date +%s)
        aws lambda invoke --function-name natbenchmark --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"' }' "out_512ram/producer_${filesize}_${run}.json" &
        aws lambda invoke --function-name natbenchmark --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"${timestamp}_1"'", "role": "consumer", "fileSize": '"$filesize"' }' "out_512ram/consumer_${filesize}_${run}.json" &
        sleep 10
    done
done
