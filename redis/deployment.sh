# Creating role
aws iam create-role --role-name lambda-vpc-role --assume-role-policy-document file://trust-policy.json
aws iam attach-role-policy --role-name lambda-vpc-role --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole
aws iam attach-role-policy --role-name lambda-vpc-role --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaVPCAccessExecutionRole

# Creating ElastiCache Cluster (group -> default VPC security group)
aws elasticache create-cache-cluster --cache-cluster-id ClusterForLambda --cache-node-type cache.t3.micro --engine redis --num-cache-nodes 1 --security-group-ids sg-06df67e1deee61365

# To query the endpoint (in CacheNodes -> Endpoint):
aws elasticache describe-cache-clusters --show-cache-node-info

# Creating function (subnet IDS: VPC -> Subnets)
aws lambda create-function \
--function-name redisbenchmark \
--role arn:aws:iam::690111777418:role/lambda-vpc-role \
--vpc-config SubnetIds=subnet-01980c108dfb69ae0,subnet-01bd5cdfba96283d1,subnet-02ace63723abfa440,SecurityGroupIds=sg-06df67e1deee61365 \
--runtime provided \
--timeout 60 \
--memory-size 2048 \
--handler redisbenchmark \
--zip-file fileb://redisbenchmark.zip

# Compiling and packaging code into zip file
make aws-lambda-package-redisbenchmark

# Updating code with newest zip
aws lambda update-function-code --function-name redisbenchmark --zip-file fileb://redisbenchmark.zip