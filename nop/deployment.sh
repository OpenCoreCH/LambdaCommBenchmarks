# Creating function
aws lambda create-function \
--function-name nopbenchmark \
--role arn:aws:iam::690111777418:role/lambda-cpp-demo \
--runtime provided \
--timeout 60 \
--memory-size 2048 \
--handler nopbenchmark \
--zip-file fileb://nopbenchmark.zip

# Compiling and packaging code into zip file
make aws-lambda-package-nopbenchmark

# Updating code with newest zip
aws lambda update-function-code --function-name nopbenchmark --zip-file fileb://nopbenchmark.zip