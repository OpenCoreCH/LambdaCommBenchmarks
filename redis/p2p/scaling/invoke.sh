#!/bin/bash
mkdir -p out/
REDIS_HOSTNAME=clusterforlambda.2ctj87.0001.euc1.cache.amazonaws.com
REDIS_PORT=6379

filesizes=(1 1000 10000 100000 1000000 10000000 100000000 500000000)
filesizes=(500000000)


for filesize in "${filesizes[@]}"
do
    for run in {1..10}
    do
        timestamp=$(date +%s)
        aws lambda invoke --function-name redisbenchmark --cli-binary-format raw-in-base64-out --payload '{"hostname":"'"$REDIS_HOSTNAME"'", "port":'"$REDIS_PORT"', "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"' }' "out/producer_${filesize}_${run}.json" &
        aws lambda invoke --function-name redisbenchmark --cli-binary-format raw-in-base64-out --payload '{"hostname":"'"$REDIS_HOSTNAME"'", "port":'"$REDIS_PORT"', "key":"'"$timestamp"'", "role": "consumer", "fileSize": '"$filesize"' }' "out/consumer_${filesize}_${run}.json" &
        if [ "$filesize" -gt "100000000" ]; then
            sleep 25
        else
            sleep 10
        fi
    done
done