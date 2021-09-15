aws iam create-role --role-name lambda-s3 --assume-role-policy-document file://trust-policy.json
aws iam attach-role-policy --role-name lambda-s3 --policy-arn arn:aws:iam::aws:policy/AmazonS3FullAccess
aws iam attach-role-policy --role-name lambda-s3 --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole

aws lambda create-function \
--function-name s3benchmark \
--role arn:aws:iam::690111777418:role/lambda-cpp-demo \
--runtime provided \
--timeout 15 \
--memory-size 128 \
--handler s3benchmark \
--zip-file fileb://s3benchmark.zip

make aws-lambda-package-s3benchmark

aws lambda update-function-code --function-name s3benchmark --zip-file fileb://s3benchmark.zip

