import boto3
import botocore
import json
import os
from concurrent.futures import ThreadPoolExecutor

cfg = botocore.config.Config(retries={'max_attempts': 0}, read_timeout=180, connect_timeout=180)
lambda_ = boto3.client('lambda', config=cfg)
n_workers = 256

def invoke_lambda(index):
    print("Invoking {}".format(index))
    data = {
        "dataset": "higgs",
        "data_bucket": "romanboe-lambdaml-data",
        "dataset_type": "dense_libsvm",
        "n_features": 28,
        "tmp_table_name": "tmp-params",
        "merged_table_name": "merged-params",
        "key_col": "key",
        "n_clusters": 3,
        "n_epochs": 10,
        "threshold": 0.0001,
        "sync_mode": "reduce",
        "n_workers": n_workers,
        "worker_index": index,
        "file": "{}".format(index)
    }

    response = lambda_.invoke(
        FunctionName='lambdaml',
        InvocationType='RequestResponse',
        LogType='Tail',
        Payload=json.dumps(data)
    )

    res_payload = response.get('Payload').read()
    
    return res_payload


if not os.path.exists("out"):
    os.makedirs("out")

with ThreadPoolExecutor(max_workers=n_workers) as executor:
    result = list(executor.map(invoke_lambda, range(n_workers)))
    for i, res in enumerate(result):
        res_text = res.decode("utf-8")
        print(res_text)
        with open("out/{}_{}.json".format(n_workers, i), "w") as txt:
            txt.write(res_text)