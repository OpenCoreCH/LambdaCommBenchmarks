#!/bin/bash
mkdir -p out/
SERVER_IP=52.59.227.123

filesizes=(500000000)

for filesize in "${filesizes[@]}"
do
    for run in {3..3}
    do
        timestamp=$(date +%s)
        aws lambda invoke --function-name natbenchmark --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"' }' "out/producer_${filesize}_${run}.json" &
        aws lambda invoke --function-name natbenchmark --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"${timestamp}_1"'", "role": "consumer", "fileSize": '"$filesize"' }' "out/consumer_${filesize}_${run}.json" &
        if [ "$filesize" -gt "100000000" ]; then
            sleep 25
        else
            sleep 10
        fi
    done
done
