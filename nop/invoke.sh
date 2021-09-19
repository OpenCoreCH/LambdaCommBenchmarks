#!/bin/bash
mkdir -p out/

consumer=256
for consumernum in $(seq 1 $consumer);
    do
    aws lambda invoke --function-name nopbenchmark  "out/${consumernum}.txt" &
    done