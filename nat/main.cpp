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
#include <tcpunch.h>
#include <fcntl.h>

using namespace aws::lambda_runtime;
char const TAG[] = "LAMBDA_ALLOC";

uint64_t send_file(int sock_fd,
                   int size);

uint64_t send_file_multiple(int sock_fds[],
                   int num_consumers,
                   int size);

uint64_t get_file(int sock_fd,
                  int size);

uint64_t timeSinceEpochMillisec()
{
  auto now = std::chrono::high_resolution_clock::now();
  auto time = now.time_since_epoch();
  return std::chrono::duration_cast< std::chrono::microseconds >(time).count();
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
    int num_consumers = 1;
    if (v.KeyExists("numConsumers")) {
        num_consumers = v.GetInteger("numConsumers");
    }
    std::cout << "Invoked handler for role " << role << " with file size " << file_size << std::endl;

        

    std::string res_json = "{ \"fileSize\": " + std::to_string(file_size) + ", \"role\": \"" + role + "\"" ;
    if (role == "producer")
    {
        int* socket_fds = new int[num_consumers];
        for (int i = 0; i < num_consumers; i++) {
            std::string prod_pairing_key = pairing_key + "_" + std::to_string(i + 1);
            socket_fds[i] = pair(prod_pairing_key, server_ip);
        }
        uint64_t upload_time;
        if (num_consumers == 1) {
            upload_time = send_file(socket_fds[0], file_size);
        } else {
            upload_time = send_file_multiple(socket_fds, num_consumers, file_size);
        }
        res_json += ", \"uploadTime\": " + std::to_string(upload_time) + " }";
        delete[] socket_fds;
    }
    else if (role == "consumer")
    {
        int socket_fd = pair(pairing_key, server_ip);
        uint64_t finished_time = get_file(socket_fd, file_size);
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

uint64_t get_file(int sock_fd,
                  int size)
{
    char* recv_buffer = new char[size];
    uint64_t download_time = 0;
    
    int recv_bytes = 0;
    while (recv_bytes < size) {
        int n = recv(sock_fd, recv_buffer + recv_bytes, size - recv_bytes, 0);
        if (n > 0) {
            recv_bytes += n;
        }
        if (n == -1) {
            std::cout << "Error: " << errno << std::endl;
            download_time = 1;
            break;
        }
    }

    if (download_time != 1) {
        download_time = timeSinceEpochMillisec();
    }

    delete[] recv_buffer;
    return download_time;
}

uint64_t send_file(int sock_fd,
                   int size)
{      
    char* pBuf = new char[size];

    uint64_t bef_upload = timeSinceEpochMillisec();

    int sent_bytes = 0;
    while (sent_bytes < size) {
        int bytes = send(sock_fd, pBuf + sent_bytes, size - sent_bytes, 0);
        sent_bytes += bytes;
        if (bytes == -1) {
            bef_upload = 0;
            break;
        }
    }
    delete[] pBuf;
    return bef_upload;
}

uint64_t send_file_multiple(int sock_fds[],
                   int num_consumers,
                   int size)
{
    char* pBuf = new char[size];

    uint64_t bef_upload = timeSinceEpochMillisec();

    int* total_sent_bytes = new int[num_consumers]();
    int curr_consumer = 0;
    bool finished = false;
    while (!finished) {
        if (total_sent_bytes[curr_consumer] < size) {
            int sent_bytes = send(sock_fds[curr_consumer], pBuf + total_sent_bytes[curr_consumer], size - total_sent_bytes[curr_consumer], 0);
            if (sent_bytes == -1) {
                std::cout << "send: " << errno << std::endl;
                bef_upload = 0;
                break;
            }
            total_sent_bytes[curr_consumer] += sent_bytes;
        }
        curr_consumer = (curr_consumer + 1) % num_consumers;
        
        // Check if all done every round
        if (curr_consumer == 0) {
            finished = true;
            for (int i = 0; i < num_consumers; i++) {
                if (total_sent_bytes[i] < size) {
                    finished = false;
                }
            }
        }
    }

    delete[] pBuf;   
    
    return bef_upload;
}