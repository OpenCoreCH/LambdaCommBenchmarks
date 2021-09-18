# Creating role
aws iam create-role --role-name lambda-s3 --assume-role-policy-document file://trust-policy.json
aws iam attach-role-policy --role-name lambda-s3 --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole

# Creating function
aws lambda create-function \
--function-name redisbenchmark \
--role arn:aws:iam::690111777418:role/lambda-cpp-demo \
--runtime provided \
--timeout 60 \
--memory-size 2048 \
--handler redisbenchmark \
--zip-file fileb://redisbenchmark.zip

# Compiling and packaging code into zip file
make aws-lambda-package-redisbenchmark

# Updating code with newest zip
aws lambda update-function-code --function-name redisbenchmark --zip-file fileb://redisbenchmark.zip