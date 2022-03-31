cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
project(redisbenchmark LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

find_package(aws-lambda-runtime REQUIRED)
find_package(AWSSDK COMPONENTS core s3)
find_package(hiredis REQUIRED)


add_executable(${PROJECT_NAME} "main.cpp")
add_executable(${PROJECT_NAME}-pingpong "main_pingpong.cpp")
target_link_libraries(${PROJECT_NAME} PUBLIC
                    AWS::aws-lambda-runtime
                    ${AWSSDK_LINK_LIBRARIES}
                    ${HIREDIS_LIBRARIES})
target_link_libraries(${PROJECT_NAME}-pingpong PUBLIC
                    AWS::aws-lambda-runtime
                    ${AWSSDK_LINK_LIBRARIES}
                    ${HIREDIS_LIBRARIES})

aws_lambda_package_target(${PROJECT_NAME})
aws_lambda_package_target(${PROJECT_NAME}-pingpong)
