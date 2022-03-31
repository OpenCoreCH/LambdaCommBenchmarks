#!/bin/bash
mkdir -p out/

SERVER_IP=18.197.228.217

#filesizes=(1 1000 10000 100000 1000000 10000000 100000000)
filesize=10000000

#s3key=$timestamp
timestamp=$(date +%s)
table="s3-benchmark"
key=$timestamp

echo $timestamp $filesize

aws lambda invoke --invocation-type Event --function-name natbenchmark --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"${timestamp}_1"'", "role": "consumer", "fileSize": '"$filesize"' }' "out/consumer_${filesize}.json" > out/consumer_${filesize}.out
aws lambda invoke --invocation-type Event --function-name natbenchmark --cli-binary-format raw-in-base64-out --payload '{"ip":"'"$SERVER_IP"'", "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"' }' "out/producer_${filesize}.json" > out/producer_${filesize}.out
