#!/bin/bash
mkdir -p out/

peers=2

for run in {1..1}
    do
        timestamp=$(date +%s)
        for peernum in $(seq 1 $peers);
            do
            peer_id=$(($peernum - 1))
            aws lambda invoke --function-name smibenchmark --cli-binary-format raw-in-base64-out --payload '{"numPeers": '"$peers"', "peerID":'"$peer_id"' }' "out/${run}_$peernum.json" &
            done
        sleep 10
    done
