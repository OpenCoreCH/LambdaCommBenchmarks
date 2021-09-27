# Creating role
aws iam create-role --role-name lambda-vpc-role --assume-role-policy-document file://trust-policy.json
aws iam attach-role-policy --role-name lambda-vpc-role --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole
aws iam attach-role-policy --role-name lambda-vpc-role --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaVPCAccessExecutionRole

# Creating ElastiCache Cluster (group -> default VPC security group)
aws elasticache create-cache-cluster --cache-cluster-id ClusterForLambda --cache-node-type cache.t3.small --engine redis --num-cache-nodes 1 --security-group-ids sg-0dd517b3a0a1f9e3e

# To query the endpoint (in CacheNodes -> Endpoint):
aws elasticache describe-cache-clusters --show-cache-node-info

# Creating function (subnet IDS: VPC -> Subnets)
aws lambda create-function \
--function-name redisbenchmark \
--role arn:aws:iam::386971375191:role/lambda-vpc-role \
--vpc-config SubnetIds=subnet-0536489c830881029,subnet-07bf8661fec5a0cb6,subnet-0a01dc2fc72af8262,SecurityGroupIds=sg-0dd517b3a0a1f9e3e \
--runtime provided \
--timeout 5 \
--memory-size 2048 \
--handler redisbenchmark \
--zip-file fileb://redisbenchmark.zip

# Compiling and packaging code into zip file
make aws-lambda-package-redisbenchmark

# Updating code with newest zip
aws lambda update-function-code --function-name redisbenchmark --zip-file fileb://redisbenchmark.zip