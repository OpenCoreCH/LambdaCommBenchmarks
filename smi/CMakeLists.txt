cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 14)
project(smibenchmark LANGUAGES CXX)

find_package(aws-lambda-runtime REQUIRED)
add_subdirectory(${PROJECT_SOURCE_DIR}/../../SMI/ smi)

add_executable(${PROJECT_NAME} "main.cpp")

target_include_directories(${PROJECT_NAME} PRIVATE ${SMI_INCLUDE_DIRS})
                           
target_link_libraries(${PROJECT_NAME} PUBLIC
                    AWS::aws-lambda-runtime
                    SMI)

aws_lambda_package_target(${PROJECT_NAME})

add_executable(smi_benchmark smi_benchmark.cpp)
target_include_directories(smi_benchmark PRIVATE ${SMI_INCLUDE_DIRS})
target_link_libraries(smi_benchmark PUBLIC SMI)
