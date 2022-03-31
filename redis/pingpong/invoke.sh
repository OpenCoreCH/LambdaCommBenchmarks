#!/bin/bash
mkdir -p out/

REDIS_HOSTNAME=clusterforlambda.h9ja7l.0001.euc1.cache.amazonaws.com
REDIS_PORT=6379

#filesizes=(1 1000 10000 100000 1000000 10000000 100000000)
filesize=100000000

timestamp=$(date +%s)
table="s3-benchmark"
key=$timestamp

echo $timestamp $filesize

aws lambda invoke --invocation-type Event --function-name redisbenchmark --cli-binary-format raw-in-base64-out --payload '{"hostname":"'"$REDIS_HOSTNAME"'", "port":'"$REDIS_PORT"', "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"' }' "out/producer_${filesize}_${run}.json" > out/consumer_${filesize}.out
aws lambda invoke --invocation-type Event --function-name redisbenchmark --cli-binary-format raw-in-base64-out --payload '{"hostname":"'"$REDIS_HOSTNAME"'", "port":'"$REDIS_PORT"', "key":"'"$timestamp"'", "role": "consumer", "fileSize": '"$filesize"' }' "out/consumer_${filesize}.json" > out/producer_${filesize}.out
