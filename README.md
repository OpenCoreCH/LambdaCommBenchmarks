# LambdaCommBenchmarks

## Requirements

- Boost
- [AWS Lambda C++ Runtime](https://aws.amazon.com/de/blogs/compute/introducing-the-c-lambda-runtime/)
- [AWS SDK for C++](https://aws.amazon.com/sdk-for-cpp/)
- [Hiredis](https://github.com/redis/hiredis)

## Build instructions
You need to build the benchmarks on an x86 Linux system.

To create the zip for AWS Lambda:

```
mkdir build
cd build
cmake .. -DBUILD_SHARED_LIBS=OFF -DENABLE_UNITY_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<optional_install_prefix>
make aws-lambda-package-<project_name>
```

## Deployment
All benchmarks contain a `deployment.sh` script with details on how to deploy them and what to start / configure on AWS.

## Running 
We provide a `invoke.sh` script for every benchmark that runs it and creates the .json files (that are parsed by the analysis script).