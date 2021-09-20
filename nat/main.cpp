#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/lambda-runtime/runtime.h>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <cstdint>
#include <iostream>
#include <hole_punching_client.h>

using boost::asio::ip::udp;
using namespace aws::lambda_runtime;
char const TAG[] = "LAMBDA_ALLOC";

uint64_t send_file(udp::socket& socket,
                   udp::endpoint peer,
                   int size);

uint64_t get_file(udp::socket&socket,
                  udp::endpoint peer);

uint64_t timeSinceEpochMillisec()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}


static invocation_response my_handler(invocation_request const &req)
{
    using namespace Aws::Utils::Json;
    JsonValue json(req.payload);
    if (!json.WasParseSuccessful())
    {
        return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
    }

    auto v = json.View();

    auto pairing_key = v.GetString("key");
    auto server_ip = v.GetString("ip");
    auto role = v.GetString("role"); // producer or consumer
    auto file_size = v.GetInteger("fileSize");
    if (v.KeyExists("waitUntil")) {
        auto wait_until = v.GetInteger("waitUntil");
        while (std::time(0) < wait_until) {
            
        }

    }
    std::cout << "Invoked handler for role " << role << " with file size " << file_size << std::endl;

    boost::asio::io_service io_service;
    udp::socket socket(io_service);
    boost::system::error_code err;
    udp::endpoint peer = pair(socket, pairing_key, server_ip);

    std::string res_json = "{ \"fileSize\": " + std::to_string(file_size) + ", \"role\": \"" + role + "\"" ;
    if (role == "producer")
    {
        uint64_t upload_time = send_file(socket, peer, file_size);
        res_json += ", \"uploadTime\": " + std::to_string(upload_time) + " }";
    }
    else if (role == "consumer")
    {
        socket.send_to(boost::asio::buffer("1"), peer, 0, err);
        
        uint64_t finished_time = get_file(socket, peer);
        res_json += ", \"finishedTime\": " + std::to_string(finished_time) + " }";
    }
    
    return invocation_response::success(res_json, "application/json");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory()
{
    return []
    {
        return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
            "console_logger", Aws::Utils::Logging::LogLevel::Info);
    };
}

int main()
{
    using namespace Aws;
    SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
    InitAPI(options);
    {
        auto handler_fn = [](aws::lambda_runtime::invocation_request const &req)
        {
            return my_handler(req);
        };
        run_handler(handler_fn);
    }
    ShutdownAPI(options);
    return 0;
}

uint64_t get_file(udp::socket&socket,
                  udp::endpoint peer)
{
    char recv_buffer[MAX_BUFFER_DATA_SIZE];
	int n = socket.receive_from(boost::asio::buffer(recv_buffer), peer);
    std::cout << "Got " << n << " bytes " << std::endl;
    int size = *((int*) recv_buffer);
    std::cout << "Total data length " << size << std::endl;
    int received_size = n - sizeof(int);
    char* out_buffer = new char[size];
    memcpy(out_buffer, recv_buffer + sizeof(int), n - sizeof(int));
    int num_packets = (size + sizeof(int) + MAX_BUFFER_DATA_SIZE - 1) / MAX_BUFFER_DATA_SIZE; // int ceil of (size + sizeof(int))/MAX_BUFFER_DATA_SIZE
    for (int i = 1; i < num_packets; i++) {
        int n = socket.receive_from(boost::asio::buffer(recv_buffer), peer);
        std::cout << "Got " << n << " bytes " << std::endl;
        memcpy(out_buffer + received_size, recv_buffer, n);
        received_size += n;
    }
    
    uint64_t after = timeSinceEpochMillisec();
    delete[] out_buffer;
	
    return after;
}

uint64_t send_file(udp::socket& socket,
                   udp::endpoint peer,
                   int size)
{
    int tot_size = size + sizeof(int);
    char* pBuf = new char[tot_size];
    *((int*) pBuf) = size;
    int num_packets = (tot_size + MAX_BUFFER_DATA_SIZE - 1) / MAX_BUFFER_DATA_SIZE; // int ceil

    uint64_t bef_upload = timeSinceEpochMillisec();
    int remaining_size = tot_size;
    for (int i = 0; i < num_packets; i++) {
        std::cout << "Sending packet " << i << std::endl;
        int packet_size = std::min(remaining_size, MAX_BUFFER_DATA_SIZE);
        socket.send_to(boost::asio::buffer(pBuf + i * MAX_BUFFER_DATA_SIZE, packet_size), peer, 0);
        remaining_size -= packet_size;
    }
    delete[] pBuf;
    return bef_upload;
}