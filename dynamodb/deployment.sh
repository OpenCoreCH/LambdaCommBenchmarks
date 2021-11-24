aws lambda create-function \
--function-name dynamobenchmark \
--role arn:aws:iam::386971375191:role/lambda-vpc-role \
--runtime provided \
--timeout 10 \
--memory-size 2048 \
--handler dynamobenchmark \
--zip-file fileb://dynamobenchmark.zip

# Compiling and packaging code into zip file
make aws-lambda-package-dynamobenchmark

# Updating code with newest zip
aws lambda update-function-code --function-name dynamobenchmark --zip-file fileb://dynamobenchmark.zip