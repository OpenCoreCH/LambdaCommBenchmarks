# Creating role
aws iam create-role --role-name lambda-s3 --assume-role-policy-document file://trust-policy.json
aws iam attach-role-policy --role-name lambda-s3 --policy-arn arn:aws:iam::aws:policy/AmazonS3FullAccess
aws iam attach-role-policy --role-name lambda-s3 --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole

# Creating function
aws lambda create-function \
--function-name s3benchmark \
--role arn:aws:iam::386971375191:role/lambda-s3 \
--runtime provided \
--timeout 5 \
--memory-size 2048 \
--handler s3benchmark \
--zip-file fileb://s3benchmark.zip

# Compiling and packaging code into zip file
make aws-lambda-package-s3benchmark

# Updating code with newest zip
aws lambda update-function-code --function-name s3benchmark --zip-file fileb://s3benchmark.zip