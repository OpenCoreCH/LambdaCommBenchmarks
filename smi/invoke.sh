#!/bin/bash
mkdir -p out/

benchmarks=(gather)
exp_peers=(4)
for peers in "${exp_peers[@]}"
do
    for benchmark in "${benchmarks[@]}"
    do
        timestamp=$(date +%s)
        for peernum in $(seq 1 $peers);
            do
            peer_id=$(($peernum - 1))
            aws lambda invoke --function-name smibenchmark --cli-binary-format raw-in-base64-out --payload '{"timestamp": "'$timestamp'", "numPeers": '"$peers"', "peerID":'"$peer_id"', "benchmark":"'$benchmark'" }' "out/${benchmark}_${peers}_$peer_id.json" &
            done
        sleep 30
    done
done
