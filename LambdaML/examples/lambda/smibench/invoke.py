import boto3
import json
import time
from concurrent.futures import ThreadPoolExecutor

lambda_ = boto3.client('lambda')
n_workers = 2

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
        "file": "{}_{}".format(index, n_workers)
    }

    response = lambda_.invoke(
        FunctionName='lambdaml',
        InvocationType='RequestResponse',
        LogType='Tail',
        Payload=json.dumps(data)
    )

    res_payload = response.get('Payload').read()
    
    return res_payload


with ThreadPoolExecutor(max_workers=n_workers) as executor:
    result = list(executor.map(invoke_lambda, range(n_workers)))
    print(result)