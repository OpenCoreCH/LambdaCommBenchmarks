#!/bin/bash
mkdir -p out/

filesizes=(1 1000 10000 100000 1000000 10000000 500000000)
filesize=10000000
consumers=(1 2 4 8 16 32 64 128 256)

for consumer in "${consumers[@]}"
    do
    mkdir -p "out/$consumer"
    for run in {1..10}
        do
            timestamp=$(date +%s)
            wait_until=$((timestamp + 10))
            aws lambda invoke --function-name s3benchmark --cli-binary-format raw-in-base64-out --payload '{"s3bucket": "romanboe-test", "s3key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"', "waitUntil": '"$wait_until"' }' "out/$consumer/producer_${filesize}_${run}.json" &
            for consumernum in $(seq 1 $consumer);
                do
                aws lambda invoke --function-name s3benchmark --cli-binary-format raw-in-base64-out --payload '{"s3bucket": "romanboe-test", "s3key":"'"$timestamp"'", "role": "consumer", "fileSize": '"$filesize"', "waitUntil": '"$wait_until"' }' "out/$consumer/consumer_${filesize}_${run}_${consumernum}.json" &
                done
            if [ "$consumer" -gt "100" ]; then
                sleep 50
            else
                sleep 20
            fi
        done
    done