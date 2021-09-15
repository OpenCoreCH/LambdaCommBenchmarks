#!/bin/bash
timestamp=$(date +%s)
filesizes=(1 1000 10000 100000 1000000 10000000 100000000)
filesize=100000

#aws lambda invoke --function-name s3benchmark --cli-binary-format raw-in-base64-out --payload '{"s3bucket": "romanboe-test", "s3key":"'"$timestamp"'", "role": "consumer", "fileSize": '"$filesize"' }' "out/consumer_${filesize}_1.json" &
aws lambda invoke --function-name s3benchmark --cli-binary-format raw-in-base64-out --payload '{"s3bucket": "romanboe-test", "s3key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"' }' "out/producer_${filesize}_1.json" &