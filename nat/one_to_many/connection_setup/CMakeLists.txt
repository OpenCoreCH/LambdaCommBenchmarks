cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
project(natbenchmarksetup LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

find_package(aws-lambda-runtime REQUIRED)
find_package(AWSSDK COMPONENTS core)
add_subdirectory(${PROJECT_SOURCE_DIR}/../../lib/CPPNatHolePunching/client/ CPPNatHolePunching)

add_executable(${PROJECT_NAME} "main.cpp")

target_include_directories(${PROJECT_NAME} PUBLIC
                           ${PROJECT_SOURCE_DIR}/../../lib/CPPNatHolePunching/client/
                           ${PROJECT_SOURCE_DIR}/../../lib/udt4/src/)

target_link_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../../lib/udt4/src/)
                           
target_link_libraries(${PROJECT_NAME} PUBLIC
                    AWS::aws-lambda-runtime
                    ${AWSSDK_LINK_LIBRARIES}
                    ${HIREDIS_LIBRARIES}
                    hole_punching_client
                    udt)

aws_lambda_package_target(${PROJECT_NAME})