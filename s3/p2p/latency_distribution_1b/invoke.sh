#!/bin/bash
mkdir -p out/

filesizes=(1 1000 10000 100000 1000000 10000000 500000000)
filesize=1

for run in {1..100}
    do
        timestamp=$(date +%s)
        aws lambda invoke --function-name s3benchmark --cli-binary-format raw-in-base64-out --payload '{"s3bucket": "romanboe-test2", "s3key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"' }' "out/producer_${filesize}_${run}.json" &
        aws lambda invoke --function-name s3benchmark --cli-binary-format raw-in-base64-out --payload '{"s3bucket": "romanboe-test2", "s3key":"'"$timestamp"'", "role": "consumer", "fileSize": '"$filesize"' }' "out/consumer_${filesize}_${run}.json" &
        sleep 5
    done