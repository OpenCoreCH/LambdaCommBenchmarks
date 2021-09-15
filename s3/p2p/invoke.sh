#!/bin/bash
timestamp=$(date +%s)

aws lambda invoke --function-name s3benchmark --cli-binary-format raw-in-base64-out --payload '{"s3bucket": "romanboe-test", "s3key":"'"$timestamp"'", "role": "consumer", "fileSize": 100 }' out/consumer_100_1.json &
aws lambda invoke --function-name s3benchmark --cli-binary-format raw-in-base64-out --payload '{"s3bucket": "romanboe-test", "s3key":"'"$timestamp"'", "role": "producer", "fileSize": 100 }' out/producer_100_1.json &