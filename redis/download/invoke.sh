#!/bin/bash

REDIS_HOSTNAME=medium.cwp3i4.ng.0001.euc1.cache.amazonaws.com
REDIS_PORT=6379

filesizes=(1 1000 10000 100000 1000000 10000000 100000000)
#for filesize in "${filesizes[@]}"
#do
#    aws lambda invoke --function-name redisbenchmark --cli-binary-format raw-in-base64-out --payload '{"hostname":"'"$REDIS_HOSTNAME"'", "port":'"$REDIS_PORT"', "key":"'"$filesize"'", "role": "producer", "fileSize": '"$filesize"' }' /dev/null
#done


for filesize in "${filesizes[@]}"
do
    for run in {1..10}
    do
        timestamp=$(date +%s)
        aws lambda invoke --function-name redisbenchmark --cli-binary-format raw-in-base64-out --payload '{"hostname":"'"$REDIS_HOSTNAME"'", "port":'"$REDIS_PORT"', "key":"'"$filesize"'", "role": "consumer", "fileSize": '"$filesize"', "downloadTime": true }' "out_medium/consumer_${filesize}_${run}.json" &
        sleep 5
    done
done