#!/bin/bash
mkdir -p out/
SERVER_IP=18.184.154.126

filesizes=(1 1000 10000 100000 1000000 10000000 100000000 500000000)

for filesize in "${filesizes[@]}"
do
    for run in {1..10}
    do
        timestamp=$(date +%s)
        aws lambda invoke --function-name natbenchmark --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"' }' "out/producer_${filesize}_${run}.json" &
        aws lambda invoke --function-name natbenchmark --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"$timestamp"'", "role": "consumer", "fileSize": '"$filesize"' }' "out/consumer_${filesize}_${run}.json" &
        if [ "$filesize" -gt "100000000" ]; then
            sleep 25
        else
            sleep 10
        fi
    done
done
