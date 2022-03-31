#!/bin/bash
mkdir -p out/

bucket='s3benchmark-eu'
#s3key=$timestamp
timestamp=$(date +%s)
table="s3-benchmark"
key=$timestamp

#filesizes=(1 1000 10000 100000 1000000 10000000)
#filesizes=(1000)
#10000 100000 1000000 10000000)
#filesizes=(100000 1000000 10000000)
filesizes=(399360)

for filesize in "${filesizes[@]}"
do
  echo "Invoke $timestamp $filesize"
  aws lambda invoke --function-name dynamodbbenchmark --invocation-type Event --cli-binary-format raw-in-base64-out --payload '{"table": "'"$table"'", "key":"'"$timestamp"'", "role": "consumer", "fileSize": '"$filesize"' }' "out/consumer_${filesize}.json" > out/consumer_${filesize}.out
  aws lambda invoke --function-name dynamodbbenchmark --invocation-type Event --cli-binary-format raw-in-base64-out --payload '{"table": "'"$table"'", "key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"' }' "out/producer_${filesize}.json" > out/producer_${filesize}.out
  #sleep 120
  #./download.sh $timestamp $filesize
done

