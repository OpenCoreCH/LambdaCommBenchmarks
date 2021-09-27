#!/bin/bash
mkdir -p out_overhead/
REDIS_HOSTNAME=clusterforlambda.cwp3i4.0001.euc1.cache.amazonaws.com
REDIS_PORT=6379

filesizes=(1 1000 10000 100000 1000000 10000000 500000000)
filesize=1000
consumers=(64)

for consumer in "${consumers[@]}"
    do
    mkdir -p "out_overhead/$consumer"
    for run in {1..3}
        do
            timestamp=$(date +%s)
            aws lambda invoke --function-name redisbenchmark --cli-binary-format raw-in-base64-out --payload '{"hostname":"'"$REDIS_HOSTNAME"'", "port":'"$REDIS_PORT"', "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"'}' "out_overhead/$consumer/producer_${filesize}_${run}.json" &
            for consumernum in $(seq 1 $consumer);
                do
                aws lambda invoke --function-name redisbenchmark --cli-binary-format raw-in-base64-out --payload '{"hostname":"'"$REDIS_HOSTNAME"'", "port":'"$REDIS_PORT"', "key":"'"$timestamp"'", "role": "consumer", "fileSize": '"$filesize"' }' "out_overhead/$consumer/consumer_${filesize}_${run}_${consumernum}.json" &
                done
            sleep 10
        done
    done