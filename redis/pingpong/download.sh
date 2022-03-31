#!/bin/bash
mkdir -p out/

timestamp=$1
filesize=$2

aws s3 cp s3://s3benchmark-eu/${timestamp}_${filesize}_producer_times results/
