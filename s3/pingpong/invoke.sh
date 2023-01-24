#!/bin/bash
mkdir -p out/

bucket='s3benchmark-eu'
s3key=$timestamp
timestamp=$(date +%s)

#filesizes=(1 1000 10000 100000 1000000 10000000)
#filesizes=(1000)
#10000 100000 1000000 10000000)
#filesizes=(100000 1000000 10000000)
filesizes=(100000000)

for filesize in "${filesizes[@]}"
do
  echo "Invoke $timestamp $filesize"
  aws lambda invoke --function-name s3benchmark --invocation-type Event --cli-binary-format raw-in-base64-out --payload '{"s3bucket": "'"$bucket"'", "s3key":"'"$timestamp"'", "role": "consumer", "fileSize": '"$filesize"' }' "out/consumer_${filesize}.json" > out/consumer_${filesize}.out
  aws lambda invoke --function-name s3benchmark --invocation-type Event --cli-binary-format raw-in-base64-out --payload '{"s3bucket": "'"$bucket"'", "s3key":"'"$timestamp"'", "role": "producer", "fileSize": '"$filesize"' }' "out/producer_${filesize}.json" > out/producer_${filesize}.out
  #sleep 120
  #./download.sh $timestamp $filesize
done

