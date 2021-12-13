#!/bin/bash
mkdir -p out/

benchmarks=(reduce allreduce scan)
benchmarks=(allreduce gather reduce scan scatter)
benchmarks=(scan)
#exp_peers=(2 4 8 16 32)
exp_peers=(256)
for peers in "${exp_peers[@]}"
do
    for benchmark in "${benchmarks[@]}"
    do
        timestamp=$(date +%s)
        for peernum in $(seq 1 $peers);
            do
            peer_id=$(($peernum - 1))
            aws lambda invoke --cli-read-timeout 600 --function-name smibenchmark --cli-binary-format raw-in-base64-out --payload '{"timestamp": "'$timestamp'", "numPeers": '"$peers"', "peerID":'"$peer_id"', "benchmark":"'$benchmark'" }' "out/${benchmark}_${peers}_$peer_id.json" &
            done
        sleep 600
    done
done
