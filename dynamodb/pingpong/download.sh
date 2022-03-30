#!/bin/bash
mkdir -p out/

timestamp=$1
filesize=$2

aws s3 cp s3://s3benchmark-eu/${timestamp}_${filesize}_producer_times results/
aws s3 cp s3://s3benchmark-eu/${timestamp}_${filesize}_producer_retries_times results/
aws s3 cp s3://s3benchmark-eu/${timestamp}_${filesize}_consumer_retries_times results/

